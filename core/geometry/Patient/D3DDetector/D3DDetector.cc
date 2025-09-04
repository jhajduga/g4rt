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
/**
 * @brief Create a D3DDetector and register it with the geometry service.
 *
 * Constructs a detector instance using the given label and immediately registers
 * the instance as a scoring component with the geometry service.
 *
 * @param label Detector identifier used for labeling and registration.
 */
D3DDetector::D3DDetector(const std::string& label) : VPatient(label), m_label(label) { AcceptGeoVisitor(Service<GeoSvc>()); }

// TODO:
// 1. Add support for multiple detectors creation by proper method (constructor with D3DTray(parent pv, "Name", position, halfSize of envelope world if needet))

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Destructor for the D3DDetector class.
 *
 * Cleans up resources by destroying the associated physical volume and performing necessary cleanup.
 */
D3DDetector::~D3DDetector() { Destroy(); }

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Print basic info about the Dose3D detector module.
 *
 * This is a placeholder/no-op function intended to report human-readable
 * information about the Dose3D module. Currently it does not modify state
 * and exists for future expansion (e.g., printing configured parameters or
 * runtime statistics).
 */
void D3DDetector::WriteInfo() { INFO_GEO("The Dose3D module info: Implement me."); }

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Release the detector's constructed physical volume and associated resources.
 *
 * Deletes the currently stored physical volume (if any) and clears the internal
 * physical-volume pointer. Safe to call when no physical volume is present.
 */
void D3DDetector::Destroy() {
  INFO_GEO("Destroing the {} volume. ", GetName());
  auto phantomVolume = GetPhysicalVolume();
  if (phantomVolume) {
    delete phantomVolume;
    SetPhysicalVolume(nullptr);
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Load detector and cell settings from the TOML configuration file.
 *
 * Parses the TOML file pointed to by GetTomlConfigFile() using the required
 * configuration prefix from GetTomlConfigPrefix() and updates the detector's
 * internal Config fields (translation, cell/voxel counts, STL paths, and
 * cell material). Also sets the global static scoring flags on D3DCell
 * (CellScorer and CellVoxelisedScorer) according to the parsed values.
 *
 * The function performs validation and will abort on fatal configuration
 * errors: it exits the process if the TOML file is missing and throws a
 * G4Exception (FatalErrorInArgument) when the configuration prefix is not
 * defined. Values not present in the TOML are replaced with sensible defaults
 * (zeros or empty strings) via toml::value_or.
 */
void D3DDetector::ParseTomlConfig() {
  auto configFile = GetTomlConfigFile();

  if (!svc::checkIfFileExist(configFile)) {
    FATAL_GEO("File {} not fount.", configFile);
    exit(1);
  }
  auto configPrefix = GetTomlConfigPrefix();
  INFO_GEO("Importing configuration from:\n{}", configFile);
  std::string configObjDetector("Detector");
  std::string configObjCell("Cell");
  if (!configPrefix.empty()) {  // here it's assummed that the config data is given with prefixed name
    configObjDetector.insert(0, configPrefix + "_");
    configObjCell.insert(0, configPrefix + "_");
  } else {
    G4String msg = "The configuration PREFIX is not defined";
    FATAL_GEO(msg.data());
    G4Exception("D3DDetector", "ParseTomlConfig", FatalErrorInArgument, msg);
  }

  auto config = toml::parse_file(configFile);

  ///
  m_config.m_translation_in_local_frame.setX(config[configObjDetector]["TranslationInLocalFrame"][0].value_or(0.0));
  m_config.m_translation_in_local_frame.setY(config[configObjDetector]["TranslationInLocalFrame"][1].value_or(0.0));
  m_config.m_translation_in_local_frame.setZ(config[configObjDetector]["TranslationInLocalFrame"][2].value_or(0.0));
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
  INFO_GEO("Importing configuration - DONE!");
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Loads default parameter values for the 3D detector configuration.
 *
 * Sets default translation, cell counts, voxelization per cell, and medium type.
 * @return `true` after successfully setting default parameters.
 */
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
/**
 * @brief Ensure detector parameterization is loaded.
 *
 * Loads parameterization from the TOML configuration when present; otherwise falls back to built-in defaults.
 * The call is idempotent: if the configuration is already marked initialized this returns immediately.
 *
 * @return true on successful load (this function always returns true when it completes).
 */
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
/**
 * @brief Replace the detector configuration and mark it initialized.
 *
 * Replaces the current configuration with the provided one and sets the
 * configuration's initialized flag to true so subsequent operations treat
 * the detector as configured.
 *
 * @param config New configuration to apply to the detector.
 */
void D3DDetector::SetConfig(const D3DDetector::Config& config) {
  m_config = config;
  m_config.m_initialized = true;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs the 3D detector geometry and its constituent cells within the simulation world.
 *
 * Depending on the configured geometry source, this method builds the detector using STL mesh data, geometry database positioning, or regular/CSV-defined cell positions. It creates and places the main detector volume if required, then instantiates and configures all `D3DCell` objects with their positions, labels, IDs, and voxelization parameters, and inserts them into the detector structure.
 *
 * @param parentWorld The parent physical volume in which the detector geometry is placed.
 */
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
      
                INFO_GEO("Defined Cell: {}", _label);
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
/**
 * @brief Updates the detector state.
 *
 * Currently a placeholder that always returns true.
 *
 * @return G4bool Always returns true.
 */
G4bool D3DDetector::Update() {
  // TODO implement me.
  return true;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Defines sensitive detectors for all cells in the detector.
 *
 * Calls the `DefineSensitiveDetector()` method on each contained `D3DCell` to enable scoring or hit collection.
 */
void D3DDetector::DefineSensitiveDetector() {
  for (auto& cell : m_d3d_cells) cell->DefineSensitiveDetector();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Registers this detector as a scoring component with the provided geometry service visitor.
 *
 * Enables external geometry services to recognize and interact with this detector for scoring purposes.
 */
void D3DDetector::AcceptGeoVisitor(GeoSvc* visitor) const { visitor->RegisterScoringComponent(this); }
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Export cell- and voxel-level positioning and ID data to a ROOT file.
 *
 * Writes flattened XYZ centers and corresponding local/global ID triplets for both
 * Cell and Voxel scoring volumes into a ROOT file named
 * "detector_scoring_volume_positioning.root" inside the provided output directory.
 * Data are stored under the "Geometry" directory with objects named
 * "<Type>Position", "<Type>ID", and "<Type>GlobalID" where <Type> is "Cell" or "Voxel".
 * The function queries scoring volumes using the run collection "Dose3D".
 *
 * @param path_to_out_dir Directory where the output ROOT file will be created.
 */
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
  INFO_GEO("Writing Dose3D Scroing Positioning to file {} - done!", file);
}
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Export per-voxel positioning for each run collection to CSV files.
 *
 * Writes one CSV file per run collection that contains voxel-level records for every voxel
 * that was scored (cell label, cell indices, voxel indices, cell center [mm], voxel center [mm]).
 * Output filenames are created as:
 *   detector_<RunCollection>_<nCellsX>x<nCellsY>x<nCellsZ>_<nVoxX>x<nVoxY>x<nVoxZ>_voxel_positioning.csv
 *
 * @param path_to_out_dir Directory where the CSV files will be written. The function will create
 *                        one file per run collection that has voxelized scoring data.
 */
void D3DDetector::ExportVoxelPositioningToCsv(const std::string& path_to_out_dir) const {
  auto run_collections = ControlPoint::GetRunCollectionNames();
  // G4cout << "DEBUG1 Writing Dose3D Voxel scroing positioning to csv..." << G4endl;
  if (run_collections.empty()) {
    WARN_GEO("D3DDetector::ExportVoxelPositioningToCsv:: Any RunCollection found.");
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
      DEBUG_GEO("D3DDetector::ExportVoxelPositioningToCsv:: No voxelisation found for {} run collection.", run_collection);
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
    // std::cout << "Writing Dose3D Voxel Scroing Positioning to file " << file << " - done!" << std::endl;
    INFO_GEO("Writing Dose3D Voxel Scroing Positioning to file {} - done!", file);
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Export cell centers and IDs to a CSV file.
 *
 * Writes one CSV file containing each cell's label, grid indices (CellIdX, CellIdY, CellIdZ)
 * and global center coordinates (in millimeters) for this detector.
 *
 * The function uses the first available run collection as a reference (cell positioning is
 * assumed identical across runs). If no run collections are available the function logs a
 * warning and returns without creating a file.
 *
 * The output filename is formed as: "detector_<NX>x<NY>x<NZ>_cell_positioning.csv" where
 * NX, NY, NZ are the configured number of cells in X, Y and Z. The CSV includes a header row
 * and columns: CellLabel, CellIdX, CellIdY, CellIdZ, CellPosX[mm], CellPosY[mm], CellPosZ[mm].
 *
 * @param path_to_out_dir Directory where the CSV file will be written.
 */
void D3DDetector::ExportCellPositioningToCsv(const std::string& path_to_out_dir) const {
  auto run_collections = ControlPoint::GetRunCollectionNames();
  if (run_collections.empty()) {
    WARN_GEO("D3DDetector::ExportCellPositioningToCsv:: Any RunCollection found.");
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
  INFO_GEO("Writing Dose3D Cell Scroing Positioning to file {} - done!", file);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Exports cell positioning data in the Gate "generic-repeater" format.
 *
 * Writes the positions of all detector cells to a text file named `generic_repeater.txt` in the specified output directory, formatted for use with the Gate simulation toolkit. Each line contains time, rotation, and cell center coordinates in millimeters.
 *
 * @param path_to_out_dir Directory where the output file will be created.
 */
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
/**
 * @brief Build a hashed map of scoring volumes for a specific run collection.
 *
 * Returns a map keyed by a deterministic hash of indices to VoxelHit objects representing
 * either every cell (Scoring::Type::Cell) or every voxel inside voxelised cells
 * (Scoring::Type::Voxel) for the given run collection.
 *
 * If this detector's label does not contain the provided run_collection string, an empty
 * map is returned. For voxel-level scoring the function skips cells that have no
 * voxelisation for the requested run collection. Each VoxelHit contains centre position,
 * local IDs, global cell IDs, volume, mass (computed from the configured cell medium density),
 * tracking-storage flag, and the cell label.
 *
 * @param run_collection Name of the run collection to query.
 * @param type Scoring level to export: cell-level or voxel-level.
 * @return std::map<std::size_t, VoxelHit> Map from hashed index to prepared VoxelHit.
 */
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
/**
 * @brief Checks if any cell in the detector has voxelized scoring enabled for the specified run collection.
 *
 * @param run_collection The name of the run collection to check for voxelized scoring.
 * @return true if at least one cell is voxelized for the given run collection; false otherwise.
 */
bool D3DDetector::IsAnyCellVoxelised(const G4String& run_collection) const {
  for (const auto& cell : m_d3d_cells) {
    if (cell->IsRunCollectionScoringVolumeVoxelised(run_collection)) return true;  // Any cell is voxelised
  }
  return false;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Determine which source will provide detector cell geometry.
 *
 * Evaluates available inputs (geometry database, configured STL filepath, and
 * CSV positioning filepath) and selects the construction mode. Side effects:
 * - If GeometryDB data is present, no further positioning is computed.
 * - If STL is "None", a regular grid is computed via ComputeRegularCellPositioning().
 * - If a positioning CSV is supplied, positions are loaded via ReadCellsPositioning().
 * - If an STL file is provided but no CSV positioning is supplied, an error is logged.
 *
 * @return std::string One of:
 * - "GeometryDB"                       — use GeometryDB-provided positioning,
 * - "Standard"                         — compute a regular grid (no STL),
 * - "PositioningFromCsv"               — use CSV positioning without STL,
 * - "StlDetectorWithPositioningFromCsv"— use STL mesh with CSV positioning,
 * - "" (empty string)                  — error (e.g., STL provided but no CSV).
 */
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
    ERROR_GEO("You can't build STL detector geometry without providing cell positioning in \".csv\" file format.");
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
/**
 * @brief Load cell center coordinates from a CSV positioning file into m_d3d_cells_positioning.
 *
 * Reads the CSV file path configured in m_config.m_stl_positioning_file_path (if the path is relative,
 * it is prefixed with PROJECT_DATA_PATH). Lines beginning with `#` are ignored; lines containing the
 * substring "layer" are treated as headers and skipped. Each data line must contain exactly three
 * comma-separated numeric values (X, Y, Z) which are converted to doubles and appended as a G4ThreeVector
 * to m_d3d_cells_positioning.
 *
 * @throws G4Exception with FatalErrorInArgument if the file cannot be opened or if any data line does not
 * contain exactly three numeric values.
 */
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
/**
 * @brief Populate m_d3d_cells_positioning with a regular 3D grid of cell centers.
 *
 * If GeometryDBReader provides cell positioning entries, those are appended unchanged.
 * Otherwise computes a centered regular grid of cell centers from the detector configuration:
 * uses each cell's nominal size plus a COVER_WIDTH envelope to derive spacing, centers the
 * grid on m_config.m_translation_in_local_frame, and fills m_d3d_cells_positioning by
 * iterating Z, then Y, then X (outer-to-inner loops) and appending each (x,y,z) position.
 *
 * The method appends positions to the existing m_d3d_cells_positioning container.
 */
void D3DDetector::ComputeRegularCellPositioning() {
  const auto& db_cells_positioning = GeometryDBReader::Instance().GetCellsPositioning();
  if (!db_cells_positioning.empty()) {
    
    INFO_GEO("Importing Cells positioning from GeometryDBReader...");
    for (const auto& cell_info : db_cells_positioning) m_d3d_cells_positioning.push_back(cell_info.com);
  } else {
    
    INFO_GEO("Creating regular Cells positioning...");
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
          INFO_GEO("Cell starting position {} {} {}",m_config.m_translation_in_local_frame.getX(), m_config.m_translation_in_local_frame.getY(), m_config.m_translation_in_local_frame.getZ());
          m_d3d_cells_positioning.emplace_back(current_x, current_y, current_z);
        }
      }
    }
  }
}
