#include "D3DDetector.hh"
#include "G4SystemOfUnits.hh"
#include "GeoSvc.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4SubtractionSolid.hh"
#include "G4ProductionCuts.hh"
#include "Services.hh"
#include "toml.hh"
#include "CADMesh.hh"
#include <vector>
#include <array>
#include "IO.hh"
#include "VPatientSD.hh"
#include "IbaImRT.hh"
#include "GeometryDBReader.hh"

std::map<std::string, std::map<std::size_t, VoxelHit>> D3DDetector::m_hashed_scoring_map_template = std::map<std::string, std::map<std::size_t, VoxelHit>>();

G4double D3DDetector::COVER_WIDTH = 1.00 * mm;

////////////////////////////////////////////////////////////////////////////////
///
D3DDetector::D3DDetector(const std::string& label) : VPatient(label), m_label(label) { AcceptGeoVisitor(Service<GeoSvc>()); }

// TODO:
// 1. Add support for multiple detectors creation by proper method (constructor with D3DTray(parent pv, "Name", position, halfSize of envelope world if needet))

////////////////////////////////////////////////////////////////////////////////
///
D3DDetector::~D3DDetector() { Destroy(); }

////////////////////////////////////////////////////////////////////////////////
///
void D3DDetector::WriteInfo() { LOGSVC_INFO("The Dose3D module info: Implement me."); }

////////////////////////////////////////////////////////////////////////////////
///
void D3DDetector::Destroy() {
  LOGSVC_INFO("Destroing the {} volume. ", GetName());
  auto phantomVolume = GetPhysicalVolume();
  if (phantomVolume) {
    delete phantomVolume;
    SetPhysicalVolume(nullptr);
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DDetector::ParseTomlConfig() {
  auto configFile = GetTomlConfigFile();

  if (!svc::checkIfFileExist(configFile)) {
    LOGSVC_CRITICAL("File {} not fount.", configFile);
    exit(1);
  }
  auto configPrefix = GetTomlConfigPrefix();
  LOGSVC_INFO("Importing configuration from:\n{}", configFile);
  std::string configObjDetector("Detector");
  std::string configObjCell("Cell");
  if (!configPrefix.empty()) {  // here it's assummed that the config data is given with prefixed name
    configObjDetector.insert(0, configPrefix + "_");
    configObjCell.insert(0, configPrefix + "_");
  } else {
    G4String msg = "The configuration PREFIX is not defined";
    LOGSVC_CRITICAL(msg.data());
    G4Exception("D3DDetector", "ParseTomlConfig", FatalErrorInArgument, msg);
  }

  auto config = toml::parse_file(configFile);

  ///
  m_config.m_translation_in_local_frame.setX(config[configObjCell]["TranslationInLocalFrame"][0].value_or(0.0));
  m_config.m_translation_in_local_frame.setY(config[configObjCell]["TranslationInLocalFrame"][1].value_or(0.0));
  m_config.m_translation_in_local_frame.setZ(config[configObjCell]["TranslationInLocalFrame"][2].value_or(0.0));

  if (Service<ConfigSvc>()->GetValue<std::string>("PatientGeometry", "EnviromentPatientEnvelop").compare("IbaImRT_Full") == 0 ||
      Service<ConfigSvc>()->GetValue<std::string>("PatientGeometry", "EnviromentPatientEnvelop").compare("IbaImRT_Box") == 0) {
    m_config.m_translation_in_local_frame += IbaImRT::IbaToLocalTranslation;
  }

  auto env_pos_x = Service<ConfigSvc>()->GetValue<double>("PatientGeometry", "PatientIsocentreX");
  auto env_pos_y = Service<ConfigSvc>()->GetValue<double>("PatientGeometry", "PatientIsocentreY");
  auto env_pos_z = Service<ConfigSvc>()->GetValue<double>("PatientGeometry", "PatientIsocentreZ");

  // Converting the order of voxelization in the Dose-3D volume to X Y Z instead of X Z Y (order of voxelization due to the method of cell placement
  // - separated production: cells and the detector)
  m_config.m_nX_cells = config[configObjDetector]["Voxelization"][0].value_or(0);
  m_config.m_nY_cells = config[configObjDetector]["Voxelization"][1].value_or(0);
  m_config.m_nZ_cells = config[configObjDetector]["Voxelization"][2].value_or(0);
  ///
  m_config.m_stl_geometry_file_path = config[configObjDetector]["Geometry"].value_or("None");
  m_config.m_stl_positioning_file_path = config[configObjDetector]["Positioning"].value_or("None");

  ///
  m_config.m_cell_nX_voxels = config[configObjCell]["Voxelization"][0].value_or(0);
  m_config.m_cell_nY_voxels = config[configObjCell]["Voxelization"][1].value_or(0);
  m_config.m_cell_nZ_voxels = config[configObjCell]["Voxelization"][2].value_or(0);
  ///
  m_config.m_cell_medium = config[configObjCell]["Medium"].value_or("");


  G4bool cell_scorer = config[configObjCell]["CellScorer"].value_or(true);
  D3DCell::CellScorer(cell_scorer);
  G4bool voxcell_scorer = config[configObjCell]["CellVoxelisedScorer"].value_or(true);
  D3DCell::CellVoxelisedScorer(voxcell_scorer);
  LOGSVC_INFO("Importing configuration - DONE!");
}

////////////////////////////////////////////////////////////////////////////////
///
G4bool D3DDetector::LoadDefaultParameterization() {
  m_config.m_translation_in_local_frame = G4ThreeVector(0.0, 0.0, 0.0);
  m_config.m_nX_cells = 4;
  m_config.m_nY_cells = 4;
  m_config.m_nZ_cells = 4;
  m_config.m_cell_nX_voxels = 4;
  m_config.m_cell_nY_voxels = 4;
  m_config.m_cell_nZ_voxels = 4;
  m_config.m_cell_medium = "PMMA";
  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
G4bool D3DDetector::LoadParameterization() {
  if (m_config.m_initialized) return true;
  if (IsTomlConfigExists()) {
    ParseTomlConfig();
  } else {
    LoadDefaultParameterization();
  }
  m_config.m_initialized = true;
  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DDetector::SetConfig(const D3DDetector::Config& config) {
  m_config = config;
  m_config.m_initialized = true;
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DDetector::Construct(G4VPhysicalVolume* parentWorld) {
  LoadParameterization();

  auto geo_type = D3DDetector::SetGeometrySource();
  G4cout << "D3D GEOMETRY CONSTRUCTION: " << geo_type << G4endl;

  // Set to store unique Y and Z values, this is for calculate dimensions for
  // StlDetectorWithPositioningFromCsv or PositioningFromCsv
  std::set<double> nY_cells;
  std::set<double> nZ_cells;

  if (geo_type.compare("StlDetectorWithPositioningFromCsv") == 0) {
    std::string path = PROJECT_DATA_PATH;
    path = path + "/" + m_config.m_stl_geometry_file_path;
    auto mesh = CADMesh::TessellatedMesh::FromSTL(path);
    G4VSolid* solid = mesh->GetSolid();
    auto Medium = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", "PMMA");
    auto dose3dCellLV = new G4LogicalVolume(solid, Medium.get(), "LVStl");
    // the placement of phantom center in the gantry (global) coordinate system that is managed by PatientGeometry class
    // here we locate the phantom box in the center of envelope box created in PatientGeometry:
    auto pv = new G4PVPlacement(nullptr, m_config.m_translation_in_local_frame, "PVStl", dose3dCellLV, parentWorld, false, 0);
  }

  auto construc_cell = [&](int ix, int iy, int iz, const std::string label, const G4ThreeVector& position) {
    std::cout << "label = " << label << "  position: " << position << std::endl;
    m_d3d_cells.push_back(new D3DCell(label, position, m_config.m_cell_medium));
    m_d3d_cells.back()->SetIDs(ix, iy, iz);
    m_d3d_cells.back()->SetNVoxels('x', m_config.m_cell_nX_voxels);
    m_d3d_cells.back()->SetNVoxels('y', m_config.m_cell_nY_voxels);
    m_d3d_cells.back()->SetNVoxels('z', m_config.m_cell_nZ_voxels);
    m_d3d_cells.back()->IPhysicalVolume::Construct(this);
  };

  // Construct individual cells:
  if (geo_type.compare("GeometryDB") == 0) {
    int _ix = 0;
    G4cout << "D3D DB CELLS CONSTRUCTION... " << G4endl;
    const auto& db_cells_positioning = GeometryDBReader::Instance().GetCellsPositioning();
    for (const auto& _cell_data : db_cells_positioning) {
      auto _label = m_label + "_Cell_" + _cell_data.sc_id;
      LOGSVC_INFO("Defined Cell: {}", _label);
      construc_cell(_ix, 0, 0, _label, _cell_data.com);
      ++_ix;
    }
  } else {
    G4cout << "D3D CELLS CONSTRUCTION... " << G4endl;
    int _ix = 0;
    int _iy = 0;
    int _iz = 0;
    int counter = 0;
    if (m_d3d_cells_positioning.empty()) G4cout << "WARNING: D3D cells positioning is empty!" << G4endl;
    for (const auto& cell_position : m_d3d_cells_positioning) {
      auto _label = m_label + "_Cell_" + std::to_string(_ix) + "_" + std::to_string(_iy) + "_" + std::to_string(_iz);
      construc_cell(_ix, _iy, _iz, _label, cell_position);
      _iz++;
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
///
G4bool D3DDetector::Update() {
  // TODO implement me.
  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DDetector::DefineSensitiveDetector() {
  for (auto& cell : m_d3d_cells) cell->DefineSensitiveDetector();
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DDetector::AcceptGeoVisitor(GeoSvc* visitor) const { visitor->RegisterScoringComponent(this); }
////////////////////////////////////////////////////////////////////////////////
/// TO BE REPAIRED
void D3DDetector::ExportPositioningToTFile(const std::string& path_to_out_dir) const {
  std::string size = std::to_string(m_config.m_nX_cells) + "x" + std::to_string(m_config.m_nY_cells) + "x" + std::to_string(m_config.m_nZ_cells);
  size += "_" + std::to_string(m_config.m_cell_nX_voxels) + "x" + std::to_string(m_config.m_cell_nY_voxels) + "x" + std::to_string(m_config.m_cell_nZ_voxels);
  std::string file = path_to_out_dir + "/detector_scoring_volume_positioning.root";
  auto tfile = IO::CreateOutputTFile(file, "Geometry");

  auto exportPositioningToTFile = [&](Scoring::Type type) {
    std::vector<double> linearized_positioning;
    std::vector<int> linearized_id_global;
    std::vector<int> linearized_id;
    auto hashed_scoring_map = GetScoringHashedMap("Dose3D", type);
    for (auto& scoring_volume : hashed_scoring_map) {
      auto& hit = scoring_volume.second;
      auto volume_centre = hit.GetCentre();
      linearized_positioning.emplace_back(volume_centre.getX());
      linearized_positioning.emplace_back(volume_centre.getY());
      linearized_positioning.emplace_back(volume_centre.getZ());
      linearized_id.emplace_back(hit.GetID(0));
      linearized_id.emplace_back(hit.GetID(1));
      linearized_id.emplace_back(hit.GetID(2));
      linearized_id_global.emplace_back(hit.GetGlobalID(0));
      linearized_id_global.emplace_back(hit.GetGlobalID(1));
      linearized_id_global.emplace_back(hit.GetGlobalID(2));
    }
    auto geo_dir = tfile->GetDirectory("Geometry");
    geo_dir->WriteObject(&linearized_positioning, (Scoring::to_string(type) + "Position").c_str());
    geo_dir->WriteObject(&linearized_id, (Scoring::to_string(type) + "ID").c_str());
    geo_dir->WriteObject(&linearized_id_global, (Scoring::to_string(type) + "GlobalID").c_str());
    tfile->Write();
  };

  exportPositioningToTFile(Scoring::Type::Cell);
  exportPositioningToTFile(Scoring::Type::Voxel);

  tfile->Close();

  std::cout << "Writing Dose3D Scroing Positioning to file " << file << " - done!" << std::endl;
  LOGSVC_INFO("Writing Dose3D Scroing Positioning to file {} - done!", file);
}
////////////////////////////////////////////////////////////////////////////////
///
void D3DDetector::ExportVoxelPositioningToCsv(const std::string& path_to_out_dir) const {
  auto run_collections = ControlPoint::GetRunCollectionNames();
  // G4cout << "DEBUG1 Writing Dose3D Voxel scroing positioning to csv..." << G4endl;
  if (run_collections.empty()) {
    LOGSVC_WARN("D3DDetector::ExportVoxelPositioningToCsv:: Any RunCollection found.");
    return;
  }
  //
  // Export takes place for each run collection that is found with cell voxelisation
  //
  std::string sep = ",";
  for (const auto& run_collection : run_collections) {
    // G4cout << "DEBUG2:: Writing Dose3D Voxel scroing positioning for RunCollection: " << run_collection << G4endl;
    auto hashed_scoring_map = GetScoringHashedMap(run_collection, Scoring::Type::Voxel);
    if (hashed_scoring_map.empty()) {
      LOGSVC_DEBUG("D3DDetector::ExportVoxelPositioningToCsv:: No voxelisation found for {} run collection.", run_collection);
      continue;
    }
    // Iterate over all cells in the detector to find any cell that is voxelised for given run collection
    VPatientSD::ScoringVolume* cell_sv = nullptr;
    for (const auto& cell : m_d3d_cells) {
      cell_sv = cell->GetSD()->GetRunCollectionReferenceScoringVolume(run_collection, false);  // TEMPORARY!!!! to be set to true
      if (cell_sv) break;
    }
    if (cell_sv) break;
    // }
    auto nvx = cell_sv->m_nVoxelsX;
    auto nvy = cell_sv->m_nVoxelsY;
    auto nvz = cell_sv->m_nVoxelsZ;
    std::string size = std::to_string(m_config.m_nX_cells) + "x" + std::to_string(m_config.m_nY_cells) + "x" + std::to_string(m_config.m_nZ_cells);
    size += "_" + std::to_string(nvx) + "x" + std::to_string(nvy) + "x" + std::to_string(nvz);
    std::string file = path_to_out_dir + "/detector_" + run_collection + "_" + size + "_voxel_positioning.csv";
    std::ofstream outFile;
    outFile.open(file.c_str(), std::ios::out);
    outFile << "CellLabel" << sep << "CellIdX" << sep << "CellIdY" << sep << "CellIdZ" << sep;
    outFile << "VoxelIdX" << sep << "VoxelIdY" << sep << "VoxelIdZ" << sep;
    outFile << "CellPosX[mm]" << sep << "CellPosY[mm]" << sep << "CellPosZ[mm]" << sep;
    outFile << "VoxelPosX[mm]" << sep << "VoxelPosY[mm]" << sep << "VoxelPosZ[mm]" << std::endl;  // data header
    // Extract voxel position for each cell from scoring map
    for (auto& scoring_volume : hashed_scoring_map) {  // this is the loop over all voxels for given run collection
      auto& hit = scoring_volume.second;
      auto hit_centre_global = hit.GetGlobalCentre();
      auto hit_centre = hit.GetCentre();
      outFile << hit.GetLabel()                   // Cell Label
              << sep << hit.GetGlobalID(0)        // Cell ID X
              << sep << hit.GetGlobalID(1)        // Cell ID Y
              << sep << hit.GetGlobalID(2)        // Cell ID Z
              << sep << hit.GetID(0)              // Voxel ID X
              << sep << hit.GetID(1)              // Voxel ID Y
              << sep << hit.GetID(2)              // Voxel ID Z
              << sep << hit_centre_global.getX()  // Cell Position X
              << sep << hit_centre_global.getY()  // Cell Position Y
              << sep << hit_centre_global.getZ()  // Cell Position Z
              << sep << hit_centre.getX()         // Voxel Position X
              << sep << hit_centre.getY()         // Voxel Position Y
              << sep << hit_centre.getZ()         // Voxel Position Z
              << std::endl;
    }
    outFile.close();
    std::cout << "Writing Dose3D Voxel Scroing Positioning to file " << file << " - done!" << std::endl;
    LOGSVC_INFO("Writing Dose3D Voxel Scroing Positioning to file {} - done!", file);
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DDetector::ExportCellPositioningToCsv(const std::string& path_to_out_dir) const {
  auto run_collections = ControlPoint::GetRunCollectionNames();
  if (run_collections.empty()) {
    LOGSVC_WARN("D3DDetector::ExportCellPositioningToCsv:: Any RunCollection found.");
    return;
  }
  // Since the cell positioning is the same for all runs, we can just take the first one
  const auto& run_collection = run_collections.at(0);

  // Export CellId to Placement mapping
  std::string sep = ",";
  std::string size = std::to_string(m_config.m_nX_cells) + "x" + std::to_string(m_config.m_nY_cells) + "x" + std::to_string(m_config.m_nZ_cells);
  std::string file = "/detector_" + size + "_cell_positioning.csv";
  file = path_to_out_dir + file;
  std::ofstream outFile;
  outFile.open(file.c_str(), std::ios::out);
  outFile << "CellLabel" << sep << "CellIdX" << sep << "CellIdY" << sep << "CellIdZ" << sep << "CellPosX[mm]" << sep << "CellPosY[mm]" << sep << "CellPosZ[mm]"
          << std::endl;  // data header
  auto hashed_scoring_map = GetScoringHashedMap(run_collection, Scoring::Type::Cell);
  for (auto& scoring_volume : hashed_scoring_map) {  // this is the loop over all cells in geometry layout
    auto& hit = scoring_volume.second;
    auto hit_centre = hit.GetCentre();
    outFile << hit.GetLabel()            // Cell Label
            << sep << hit.GetID(0)       // Cell ID X
            << sep << hit.GetID(1)       // Cell ID Y
            << sep << hit.GetID(2)       // Cell ID Z
            << sep << hit_centre.getX()  // Cell Position X
            << sep << hit_centre.getY()  // Cell Position Y
            << sep << hit_centre.getZ()  // Cell Position Z
            << std::endl;
  }
  outFile.close();
  LOGSVC_INFO("Writing Dose3D Cell Scroing Positioning to file {} - done!", file);
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DDetector::ExportToGateCsv(const std::string& path_to_out_dir) const {
  // Export Cell positioning to gate "generic-repeater" format
  std::string gateSep = " ";
  std::string fileName = "/generic_repeater.txt";
  fileName = path_to_out_dir + fileName;
  std::ofstream outGateFile;
  outGateFile.open(fileName.c_str(), std::ios::out);
  outGateFile << "######    time [s]    rotationAngle[deg]    rotationAxisX    rotationAxisY    rotationAxisZ    CellPosX[mm]    CellPosY[mm]    CellPosZ[mm]" << std::endl;
  outGateFile << "Time     s\nRotation deg\nTranslation mm" << std::endl;
  for (const auto& cell : m_d3d_cells) {
    auto label = cell->GetName();
    auto centre = cell->GetCentre();
    auto rotationAngle = cell->GetPhysicalVolume()->GetRotation();
    auto rotation = cell->GetPhysicalVolume()->GetObjectRotation();
    auto posX = centre.getX() / CLHEP::mm;
    auto posY = centre.getY() / CLHEP::mm;
    auto posZ = centre.getZ() / CLHEP::mm;
    outGateFile << 0.0 << gateSep << 0.0 << gateSep << 0 << gateSep << 1 << gateSep << 0 << gateSep << posX << gateSep << posY << gateSep << posZ << std::endl;
  }
  outGateFile.close();
}

////////////////////////////////////////////////////////////////////////////////
///
std::map<std::size_t, VoxelHit> D3DDetector::GetScoringHashedMap(const G4String& run_collection, Scoring::Type type) const {
  if (!m_label.contains(run_collection)) return std::map<std::size_t, VoxelHit>();  // return empty map

  // G4cout<<"GetScoringHashedMap for " << run_collection << " / " <<Scoring::to_string(type)<<G4endl;
  std::map<std::size_t, VoxelHit> hashed_map_scoring;
  auto Medium = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", m_config.m_cell_medium);
  for (const auto& cell : m_d3d_cells) {
    auto centre = cell->GetGlobalCentre();
    auto cIdX = cell->GetIdX();
    auto cIdY = cell->GetIdY();
    auto cIdZ = cell->GetIdZ();

    if (type == Scoring::Type::Voxel) {
      // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
      // !!! By now IsAnyCellVoxelised(run_collection) && causes
      // std::out_of_range, wołamy o mape której nie ma...
      auto cell_sv = cell->GetSD()->GetRunCollectionReferenceScoringVolume(run_collection, true);
      if (cell_sv == nullptr)  // no voxelisation for this cell, continue
        continue;
      auto nvx = cell_sv->m_nVoxelsX;
      auto nvy = cell_sv->m_nVoxelsY;
      auto nvz = cell_sv->m_nVoxelsZ;

      for (int ix = 0; ix < nvx; ix++) {
        for (int iy = 0; iy < nvy; iy++) {
          for (int iz = 0; iz < nvz; iz++) {
            auto voxelHash = svc::getHashedStrFromIndexes({cIdX, cIdY, cIdZ, ix, iy, iz});
            hashed_map_scoring[voxelHash] = VoxelHit();
            hashed_map_scoring[voxelHash].SetCentre(cell_sv->GetVoxelCentre(ix, iy, iz));
            hashed_map_scoring[voxelHash].SetId(ix, iy, iz);
            hashed_map_scoring[voxelHash].SetGlobalId(cIdX, cIdY, cIdZ);
            hashed_map_scoring[voxelHash].SetStoreTracks(Service<ConfigSvc>()->GetValue<bool>("RunSvc", "StoreTracks"));
            hashed_map_scoring[voxelHash].SetVolume(cell_sv->GetVoxelVolume());
            hashed_map_scoring[voxelHash].SetMass(Medium->GetDensity() * hashed_map_scoring[voxelHash].GetVolume());
            hashed_map_scoring[voxelHash].SetLabel(cell->GetName());
          }
        }
      }
    } else if (type == Scoring::Type::Cell) {
      auto cellHash = svc::getHashedStrFromIndexes({cIdX, cIdY, cIdZ});
      hashed_map_scoring[cellHash] = VoxelHit();
      hashed_map_scoring[cellHash].SetCentre(centre);
      hashed_map_scoring[cellHash].SetId(cIdX, cIdY, cIdZ);
      hashed_map_scoring[cellHash].SetGlobalId(cIdX, cIdY, cIdZ);  // Id == GlobalId
      hashed_map_scoring[cellHash].SetStoreTracks(Service<ConfigSvc>()->GetValue<bool>("RunSvc", "StoreTracks"));
      hashed_map_scoring[cellHash].SetVolume(GetCellVolume());
      hashed_map_scoring[cellHash].SetMass(Medium->GetDensity() * hashed_map_scoring[cellHash].GetVolume());
      hashed_map_scoring[cellHash].SetLabel(cell->GetName());
      // hashed_map_scoring[cellHash].Print();
    }
  }
  return hashed_map_scoring;
}

////////////////////////////////////////////////////////////////////////////////
///
bool D3DDetector::IsAnyCellVoxelised(const G4String& run_collection) const {
  for (const auto& cell : m_d3d_cells) {
    if (cell->IsRunCollectionScoringVolumeVoxelised(run_collection)) return true;  // Any cell is voxelised
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
///
std::string D3DDetector::SetGeometrySource() {
  auto geo_type = "";

  const auto& db_cells_positioning = GeometryDBReader::Instance().GetCellsPositioning();
  if (!db_cells_positioning.empty()) {
    return geo_type = "GeometryDB";
  }

  if ((m_config.m_stl_geometry_file_path.compare("None") == 0)) {
    ComputeRegularCellPositioning();
    return geo_type = "Standard";
  }

  else if ((m_config.m_stl_geometry_file_path.compare("None") != 0) && (m_config.m_stl_positioning_file_path.compare("None") == 0)) {
    LOGSVC_ERROR("You can't build STL detector geometry without providing cell positioning in \".csv\" file format.");
    return geo_type;
  }

  else if ((m_config.m_stl_geometry_file_path.compare("None") == 0) && (m_config.m_stl_positioning_file_path.compare("None") != 0)) {
    ReadCellsPositioning();
    return geo_type = "PositioningFromCsv";
  }

  else if ((m_config.m_stl_geometry_file_path.compare("None") != 0) && (m_config.m_stl_positioning_file_path.compare("None") != 0)) {
    ReadCellsPositioning();
    return geo_type = "StlDetectorWithPositioningFromCsv";
  }
  return geo_type;
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DDetector::ReadCellsPositioning() {
  std::string line;
  if (m_config.m_stl_positioning_file_path.at(0) != '/') {
    std::string path = PROJECT_DATA_PATH;
    m_config.m_stl_positioning_file_path = path + "/" + m_config.m_stl_positioning_file_path;
  }
  std::ifstream file(m_config.m_stl_positioning_file_path.data());
  bool is_first_layer = true;
  if (file.is_open()) {  // check if file exists
    while (getline(file, line)) {
      if (line.length() > 0 && line.at(0) != '#') {
        if (line.find("layer") == std::string::npos) {
          std::istringstream ss(line);
          std::string svalue;
          std::vector<double> xyz;
          while (getline(ss, svalue, ',')) {
            xyz.emplace_back(std::strtod(svalue.c_str(), nullptr));
          }
          if (xyz.size() != 3) {
            G4String description = "Wrong G4ThreeVector parameters number read-in from FILE!";
            G4Exception("D3DDetector", "G4ThreeVector", FatalErrorInArgument, description.data());
          }
          m_d3d_cells_positioning.emplace_back(xyz.at(0), xyz.at(1), xyz.at(2));
        }
        is_first_layer = false;
      }
    }
  } else {
    G4String description = "File doesn't exists!";
    G4Exception("D3DDetector", "FILE", FatalErrorInArgument, description.data());
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DDetector::ComputeRegularCellPositioning() {
  const auto& db_cells_positioning = GeometryDBReader::Instance().GetCellsPositioning();
  if (!db_cells_positioning.empty()) {
    LOGSVC_INFO("Importing Cells positioning from GeometryDBReader...");
    for (const auto& cell_info : db_cells_positioning) m_d3d_cells_positioning.push_back(cell_info.com);
  } else {
    LOGSVC_INFO("Creating regular Cells positioning...");
    G4double size_x = D3DCell::SIZE.getX() + 2 * D3DDetector::COVER_WIDTH;
    G4double size_y = D3DCell::SIZE.getY() + 2 * D3DDetector::COVER_WIDTH;
    G4double size_z = D3DCell::SIZE.getZ() + 2 * D3DDetector::COVER_WIDTH;
    G4double init_x = m_config.m_translation_in_local_frame.getX() - (m_config.m_nX_cells - 1) * size_x / 2.;
    G4double init_y = m_config.m_translation_in_local_frame.getY() - (m_config.m_nY_cells - 1) * size_y / 2.;
    G4double init_z = m_config.m_translation_in_local_frame.getZ() + (m_config.m_nZ_cells - 1) * size_z / 2.;
    for (int iz = 0; iz < m_config.m_nZ_cells; ++iz) {
      auto current_z = init_z + iz * size_x;
      for (int iy = 0; iy < m_config.m_nY_cells; ++iy) {
        auto current_y = init_y + iy * size_y;
        for (int ix = 0; ix < m_config.m_nX_cells; ++ix) {
          auto current_x = init_x + ix * size_z;
          m_d3d_cells_positioning.emplace_back(current_x, current_y, current_z);
        }
      }
    }
  }
}
