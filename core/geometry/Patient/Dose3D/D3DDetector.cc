#include "D3DDetector.hh"
#include "G4SystemOfUnits.hh"
#include "GeoSvc.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4SubtractionSolid.hh"
#include "G4ProductionCuts.hh"
#include "Services.hh"
#include "toml.hh"
#include "TH2Poly.h"
#include "TFile.h"
#include "TTree.h"
#include "CADMesh.hh"
#include <vector>
#include <array>
#include "IO.hh"
#include "VPatientSD.hh"


std::map<std::string, std::map<std::size_t, VoxelHit>> D3DDetector::m_hashed_scoring_map_template = std::map<std::string, std::map<std::size_t, VoxelHit>>();

////////////////////////////////////////////////////////////////////////////////
///
D3DDetector::D3DDetector(const std::string& label): VPatient(label), m_label(label) {
    AcceptGeoVisitor(Service<GeoSvc>());
}


// TODO:
// 1. Add support for multiple detectors creation by proper method (constructor with D3DTray(parent pv, "Name", position, halfSize of envelope world if needet))

////////////////////////////////////////////////////////////////////////////////
///
D3DDetector::~D3DDetector() {
  Destroy();
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DDetector::WriteInfo() {
  LOGSVC_INFO("The Dose3D module info: Implement me.");
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DDetector::Destroy() {
  LOGSVC_INFO("Destroing the {} volume. ",GetName());
  auto phantomVolume = GetPhysicalVolume();
  if (phantomVolume) {
    delete phantomVolume;
    SetPhysicalVolume(nullptr);
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DDetector::ParseTomlConfig(){
  auto configFile = GetTomlConfigFile();

  if (!svc::checkIfFileExist(configFile)) {
    LOGSVC_CRITICAL("File {} not fount.", configFile);
    exit(1);
  }
  auto configPrefix = GetTomlConfigPrefix();
  LOGSVC_INFO("Importing configuration from:\n{}",configFile);
  std::string configObjDetector("Detector");
  std::string configObjLayer("Layer");
  std::string configObjCell("Cell");
  if(!configPrefix.empty()){ // here it's assummed that the config data is given with prefixed name
    configObjDetector.insert(0,configPrefix+"_");
    configObjLayer.insert(0,configPrefix+"_");
    configObjCell.insert(0,configPrefix+"_");
  }
  else {
    G4String msg = "The configuration PREFIX is not defined";
    LOGSVC_CRITICAL(msg.data());
    G4Exception("D3DDetector", "ParseTomlConfig", FatalErrorInArgument, msg);
  }

  auto config = toml::parse_file(configFile);

  ///
  m_config.m_top_position_in_env.setX(config[configObjDetector]["TopPositionInEnv"][0].value_or(0.0));
  m_config.m_top_position_in_env.setY(config[configObjDetector]["TopPositionInEnv"][1].value_or(0.0));
  m_config.m_top_position_in_env.setZ(config[configObjDetector]["TopPositionInEnv"][2].value_or(0.0));

  auto env_pos_x = Service<ConfigSvc>()->GetValue<double>("PatientGeometry", "EnviromentPositionX");
  auto env_pos_y = Service<ConfigSvc>()->GetValue<double>("PatientGeometry", "EnviromentPositionY");
  auto env_pos_z = Service<ConfigSvc>()->GetValue<double>("PatientGeometry", "EnviromentPositionZ");

  m_patient_top_position_in_world_env = G4ThreeVector(env_pos_x,env_pos_y,env_pos_z) + m_config.m_top_position_in_env;

  // Converting the order of voxelization in the Dose-3D volume to X Y Z instead of X Z Y (order of voxelization due to the method of cell placement 
  // - separated production: cells, layers and the detector)
  m_config.m_nX_cells = config[configObjDetector]["Voxelization"][0].value_or(0);
  m_config.m_nY_cells = config[configObjDetector]["Voxelization"][1].value_or(0);
  m_config.m_nZ_cells = config[configObjDetector]["Voxelization"][2].value_or(0);
  ///
  m_config.m_stl_geometry_file_path = config[configObjDetector]["Geometry"].value_or("None");
  m_config.m_in_layer_positioning_module = config[configObjLayer]["Positioning"].value_or("None");

  ///
  m_config.m_mrow_shift = config[configObjLayer]["MRowShift"].value_or(false);
  m_config.m_mlayer_shift = config[configObjLayer]["MLayerShift"].value_or(false);
  ///
  m_config.m_cell_nX_voxels = config[configObjCell]["Voxelization"][0].value_or(0);
  m_config.m_cell_nY_voxels = config[configObjCell]["Voxelization"][1].value_or(0);
  m_config.m_cell_nZ_voxels = config[configObjCell]["Voxelization"][2].value_or(0);
  /// 
  m_config.m_cell_medium = config[configObjCell]["Medium"].value_or("");

  m_tracks_analysis = config[configObjCell]["TracksAnalysis"].value_or(false);

  G4bool cell_scorer = config[configObjCell]["CellScorer"].value_or(true);
  D3DCell::CellScorer(cell_scorer);
  G4bool voxcell_scorer = config[configObjCell]["CellVoxelisedScorer"].value_or(true);
  D3DCell::CellVoxelisedScorer(voxcell_scorer);
  LOGSVC_INFO("Importing configuration - DONE!");
  }


////////////////////////////////////////////////////////////////////////////////
///
G4bool D3DDetector::LoadDefaultParameterization(){
  m_config.m_top_position_in_env = G4ThreeVector(0.0,0.0,0.0);
  m_config.m_nX_cells = 4;
  m_config.m_nY_cells = 4;
  m_config.m_nZ_cells = 4;
  m_config.m_cell_nX_voxels = 4;
  m_config.m_cell_nY_voxels = 4;
  m_config.m_cell_nZ_voxels = 4;
  m_config.m_mrow_shift = true;
  m_config.m_mlayer_shift = true;
  m_config.m_cell_medium = "PMMA";
  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
G4bool D3DDetector::LoadParameterization(){
  if(m_config.m_initialized)
    return true;
  if(IsTomlConfigExists()){
    ParseTomlConfig();
  }
  else{
    LoadDefaultParameterization();
  }
  m_config.m_initialized=true;
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
void D3DDetector::Construct(G4VPhysicalVolume *parentWorld) {
  LoadParameterization();
  auto size = D3DCell::SIZE;
  auto cover = D3DMLayer::COVER_WIDTH;

  auto geo_type = D3DDetector::SetGeometrySource();
  G4cout<< "Zaczyna konstrukcję... "<< geo_type <<G4endl;

  // Set to store unique Y and Z values, this is for calculate dimensions for
  // StlDetectorWithPositioningFromCsv or PositioningFromCsv
  std::set<double> nY_cells;
  std::set<double> nZ_cells;

  auto processLayerDimensionality = [&nY_cells, &nZ_cells](const std::vector<G4ThreeVector>& vecs) {
      for(const auto& vec:vecs){
        nY_cells.insert(vec.getY());
        nZ_cells.insert(vec.getZ());
      }
  };

  if(geo_type.compare("StlDetectorWithPositioningFromCsv")==0){
    std::string path = PROJECT_DATA_PATH;
    path = path + "/" + m_config.m_stl_geometry_file_path;
    auto mesh = CADMesh::TessellatedMesh::FromSTL(path);
    G4VSolid* solid = mesh->GetSolid();
    auto Medium = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", "PMMA");
    auto dose3dCellLV = new G4LogicalVolume(solid, Medium.get(), "LVStl");
    // the placement of phantom center in the gantry (global) coordinate system that is managed by PatientGeometry class
    // here we locate the phantom box in the center of envelope box created in PatientGeometry:
    auto pv = new G4PVPlacement(nullptr, m_config.m_top_position_in_env, "PVStl", dose3dCellLV, parentWorld, false, 0);
    // auto pv = GetPhysicalVolume();


    //
    int i_layer = 0;
    for(const auto& cells_in_layer_positioning : m_d3d_cells_in_layers_positioning ){
      G4cout << "D3DDetector:: \""<<GetName()<<"\" instantiate #layer:" << i_layer << " with #cells: " << cells_in_layer_positioning.size() << G4endl;
      auto label = m_label+"_Layer_"+std::to_string(i_layer);
      m_d3d_layers.push_back(new D3DMLayer(label, m_config.m_cell_medium, cells_in_layer_positioning));
      m_d3d_layers.back()->SetId(i_layer++);
      m_d3d_layers.back()->SetPosition(m_config.m_top_position_in_env);
      m_d3d_layers.back()->SetCellNVoxels('x',m_config.m_cell_nX_voxels);
      m_d3d_layers.back()->SetCellNVoxels('y',m_config.m_cell_nY_voxels);
      m_d3d_layers.back()->SetCellNVoxels('z',m_config.m_cell_nZ_voxels);
      m_d3d_layers.back()->SetTracksAnalysis(m_tracks_analysis);
      m_d3d_layers.back()->IPhysicalVolume::Construct(this);
      processLayerDimensionality(cells_in_layer_positioning);
    }
  }

  if(geo_type.compare("PositioningFromCsv")==0){
    int i_layer = 0;
    for(const auto& cells_in_layer_positioning : m_d3d_cells_in_layers_positioning ){
      G4cout << "D3DDetector:: \""<<GetName()<<"\" instantiate #layer:" << i_layer << " with #cells: "
      << cells_in_layer_positioning.size() << G4endl;
      auto label = m_label+"_Layer_"+std::to_string(i_layer);
      m_d3d_layers.push_back(new D3DMLayer(label, m_config.m_cell_medium, cells_in_layer_positioning));
      m_d3d_layers.back()->SetId(i_layer++);
      m_d3d_layers.back()->SetPosition(m_config.m_top_position_in_env);
      m_d3d_layers.back()->SetCellNVoxels('x',m_config.m_cell_nX_voxels);
      m_d3d_layers.back()->SetCellNVoxels('y',m_config.m_cell_nY_voxels);
      m_d3d_layers.back()->SetCellNVoxels('z',m_config.m_cell_nZ_voxels);
      m_d3d_layers.back()->SetTracksAnalysis(m_tracks_analysis);
      m_d3d_layers.back()->IPhysicalVolume::Construct(this);
      processLayerDimensionality(cells_in_layer_positioning);
    }
  }
  if(geo_type.compare("PositioningFromCsv")==0 ||
     geo_type.compare("StlDetectorWithPositioningFromCsv")==0){
    m_config.m_nX_cells = m_d3d_cells_in_layers_positioning.size();
    m_config.m_nY_cells = nY_cells.size();
    m_config.m_nZ_cells = nZ_cells.size();
  }
  ///////////////////////////////////////////
  /// Building standard procedural generated geometry
  if(geo_type.compare("Standard")==0){
      
    G4double layer_width = size + (2*cover);

    auto rotation = parentWorld->GetObjectRotation();

    G4double init_y = m_config.m_top_position_in_env.getY() - (m_config.m_nY_cells-1) * layer_width/2. ; 
    for(int i_layer = 0; i_layer < m_config.m_nY_cells; ++i_layer ){
      auto label = m_label+"_Layer_"+std::to_string(i_layer);
      G4double init_x = m_config.m_top_position_in_env.getX() - (m_config.m_nX_cells-1) * layer_width/2.;
      G4double init_z = m_config.m_top_position_in_env.getZ() + layer_width/2.;
      G4cout << "[DEBUG]:: >>> >>> D3DDetector:: Z translation: " << init_z << G4endl;
      //_______________________________________
      // Take into account shifts related to layerss parity  
      // within the detector assembly 
      if (i_layer>0)
        init_y+= layer_width;
      if(m_config.m_mrow_shift && i_layer%2)
        init_x += layer_width/2.;
      m_d3d_layers.push_back(new D3DMLayer(label,
                  m_config.m_cell_medium,
                  m_config.m_mlayer_shift));
      m_d3d_layers.back()->SetId(i_layer);
      m_d3d_layers.back()->SetNCells('x',m_config.m_nX_cells);
      m_d3d_layers.back()->SetNCells('z',m_config.m_nZ_cells);
      ///
      m_d3d_layers.back()->SetCellNVoxels('x',m_config.m_cell_nX_voxels);
      m_d3d_layers.back()->SetCellNVoxels('y',m_config.m_cell_nY_voxels);
      m_d3d_layers.back()->SetCellNVoxels('z',m_config.m_cell_nZ_voxels);
      ///
      m_d3d_layers.back()->SetPosition('x',init_x);
      m_d3d_layers.back()->SetPosition('y',init_y);
      m_d3d_layers.back()->SetPosition('z',init_z);
      ///
      m_d3d_layers.back()->SetTracksAnalysis(m_tracks_analysis);
      ///
      m_d3d_layers.back()->IPhysicalVolume::Construct(this);
    }

    // G4RotationMatrix * RotMat = new G4RotationMatrix();

    // auto medium = Service<ConfigSvc>()->GetValue<G4MaterialSPtr>("MaterialsSvc", "steel2");
    // auto my_screw = new G4Tubs("screwone",0,2.05*mm,12.0*mm,0,360*degree);
    // auto pBoxLog1 = new G4LogicalVolume(my_screw, medium.get(), "screwoneLV1");
    // SetPhysicalVolume(new G4PVPlacement(RotMat, G4ThreeVector(20.725,20.725,0), "ScrewPhys1", pBoxLog1, parentWorld, false, 0));

    // auto pBoxLog2 = new G4LogicalVolume(my_screw, medium.get(), "screwoneLV2");
    // SetPhysicalVolume(new G4PVPlacement(RotMat, G4ThreeVector(-20.725,20.725,0), "ScrewPhys2", pBoxLog2, parentWorld, false, 0));

    // auto pBoxLog3 = new G4LogicalVolume(my_screw, medium.get(), "screwoneLV3");
    // SetPhysicalVolume(new G4PVPlacement(RotMat, G4ThreeVector(20.725,-20.725,0), "ScrewPhys3", pBoxLog3, parentWorld, false, 0));

    // auto pBoxLog4 = new G4LogicalVolume(my_screw, medium.get(), "screwoneLV4");
    // SetPhysicalVolume(new G4PVPlacement(RotMat, G4ThreeVector(-20.725,-20.725,0), "ScrewPhys4", pBoxLog4, parentWorld, false, 0));

    // auto Medium = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_Cu");
    // auto coverBox = new G4Box("CoverBox", 45.0*mm, 45.0*mm, 7.0*mm);
    // auto dcoverLV = new G4LogicalVolume(coverBox, Medium.get(), "CoverBoxLV");
    // SetPhysicalVolume(new G4PVPlacement(nullptr, G4ThreeVector(0.0,0.0,-213.0), "CoverBoxPV", dcoverLV, parentWorld, false, 0));

    // auto Medium2 = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_Zn");
    // auto coverBox2 = new G4Box("Cover2Box", 45.0*mm, 45.0*mm, 13.0*mm);
    // auto dcover2LV = new G4LogicalVolume(coverBox2, Medium2.get(), "CoverBox2LV");
    // SetPhysicalVolume(new G4PVPlacement(nullptr, G4ThreeVector(0.0,0.0,-193.0), "CoverBox2PV", dcover2LV, parentWorld, false, 0));

    // auto dcover3LV = new G4LogicalVolume(coverBox, Medium.get(), "CoverBox3LV");
    // SetPhysicalVolume(new G4PVPlacement(nullptr, G4ThreeVector(0.0,0.0,-173.0), "CoverBox3PV", dcover3LV, parentWorld, false, 0));

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
void D3DDetector::DefineSensitiveDetector(){
  for (auto& mLayer : m_d3d_layers)
    mLayer->DefineSensitiveDetector();
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DDetector::AcceptGeoVisitor(GeoSvc *visitor) const {
  visitor->RegisterScoringComponent(this);
}
////////////////////////////////////////////////////////////////////////////////
/// TO BE REPAIRED
void D3DDetector::ExportPositioningToTFile(const std::string& path_to_out_dir) const {
  std::string size = std::to_string(m_config.m_nX_cells)+"x"+std::to_string(m_config.m_nY_cells)+"x"+std::to_string(m_config.m_nZ_cells);
  size += "_"+std::to_string(m_config.m_cell_nX_voxels)+"x"+std::to_string(m_config.m_cell_nY_voxels)+"x"+std::to_string(m_config.m_cell_nZ_voxels);
  std::string file = path_to_out_dir + "/detector_scoring_volume_positioning.root";
  auto tfile = IO::CreateOutputTFile(file,"Geometry");

  auto exportPositioningToTFile = [&](Scoring::Type type){
    std::vector<double> linearized_positioning;
    std::vector<int> linearized_id_global;
    std::vector<int> linearized_id;
    auto hashed_scoring_map = GetScoringHashedMap("Dose3D",type);
    for(auto& scoring_volume : hashed_scoring_map){
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
    geo_dir->WriteObject(&linearized_positioning, (Scoring::to_string(type)+"Position").c_str());
    geo_dir->WriteObject(&linearized_id, (Scoring::to_string(type)+"ID").c_str());
    geo_dir->WriteObject(&linearized_id_global, (Scoring::to_string(type)+"GlobalID").c_str());
    tfile->Write();
  };

  exportPositioningToTFile(Scoring::Type::Cell);
  exportPositioningToTFile(Scoring::Type::Voxel);

  tfile->Close();

  std::cout << "Writing Dose3D Scroing Positioning to file " <<file<< " - done!" << std::endl;
  LOGSVC_INFO("Writing Dose3D Scroing Positioning to file {} - done!",file);
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
  for(const auto& run_collection : run_collections){
    // G4cout << "DEBUG2:: Writing Dose3D Voxel scroing positioning for RunCollection: " << run_collection << G4endl;
    auto hashed_scoring_map = GetScoringHashedMap(run_collection,Scoring::Type::Voxel);
    if(hashed_scoring_map.empty()){
      LOGSVC_DEBUG("D3DDetector::ExportVoxelPositioningToCsv:: No voxelisation found for {} run collection.",run_collection);
      continue;
    }
    //Iterate over all cells in the detector to find any cell that is voxelised for given run collection
    VPatientSD::ScoringVolume* cell_sv = nullptr;
    for(const auto& mLayer: m_d3d_layers){
      for(const auto& cell: mLayer->GetCells()){
        cell_sv = cell->GetSD()->GetRunCollectionReferenceScoringVolume(run_collection,false); // TEMPORARY!!!! to be set to true
        if(cell_sv) break;
      }
      if(cell_sv) break;
    }
    auto nvx = cell_sv->m_nVoxelsX;
    auto nvy = cell_sv->m_nVoxelsY;
    auto nvz = cell_sv->m_nVoxelsZ;
    std::string size = std::to_string(m_config.m_nX_cells)+"x"+std::to_string(m_config.m_nY_cells)+"x"+std::to_string(m_config.m_nZ_cells);
    size += "_"+std::to_string(nvx)+"x"+std::to_string(nvy)+"x"+std::to_string(nvz);
    std::string file = path_to_out_dir + "/detector_"+run_collection +"_"+size+"_voxel_positioning.csv";
    std::ofstream outFile;
    outFile.open(file.c_str(), std::ios::out);
    outFile <<"CellIdX"<<sep<<"CellIdY"<<sep<<"CellIdZ"<<sep;
    outFile <<"VoxelIdX"<<sep<<"VoxelIdY"<<sep<<"VoxelIdZ"<<sep;
    outFile <<"CellPosX[mm]"<<sep<<"CellPosY[mm]"<<sep<<"CellPosZ[mm]"<<sep;
    outFile <<"VoxelPosX[mm]"<<sep<<"VoxelPosY[mm]"<<sep<<"VoxelPosZ[mm]"<< std::endl; // data header
    // Extract voxel position for each cell from scoring map
    for(auto& scoring_volume : hashed_scoring_map){ // this is the loop over all voxels for given run collection
      auto& hit = scoring_volume.second;
      auto hit_centre_global = hit.GetGlobalCentre();
      auto hit_centre = hit.GetCentre();
      outFile   << hit.GetGlobalID(0) // Cell ID X
        << sep  << hit.GetGlobalID(1) // Cell ID Y
        << sep  << hit.GetGlobalID(2) // Cell ID Z
        << sep  << hit.GetID(0) // Voxel ID X
        << sep  << hit.GetID(1) // Voxel ID Y
        << sep  << hit.GetID(2) // Voxel ID Z
        << sep  << hit_centre_global.getX() // Cell Position X
        << sep  << hit_centre_global.getY() // Cell Position Y
        << sep  << hit_centre_global.getZ() // Cell Position Z
        << sep  << hit_centre.getX() // Voxel Position X
        << sep  << hit_centre.getY() // Voxel Position Y
        << sep  << hit_centre.getZ() // Voxel Position Z
        << std::endl;
    }
    outFile.close();
    std::cout << "Writing Dose3D Voxel Scroing Positioning to file " <<file<< " - done!" << std::endl;
    LOGSVC_INFO("Writing Dose3D Voxel Scroing Positioning to file {} - done!",file);
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
  std::string size = std::to_string(m_config.m_nX_cells)+"x"+std::to_string(m_config.m_nY_cells)+"x"+std::to_string(m_config.m_nZ_cells);
  std::string file = "/detector_" + size + "_cell_positioning.csv";
  file = path_to_out_dir + file;
  std::ofstream outFile;
  outFile.open(file.c_str(), std::ios::out);
  outFile << "CellIdX"<<sep<<"CellIdY"<<sep<<"CellIdZ"<<sep<<"CellPosX[mm]"<<sep<<"CellPosY[mm]"<<sep<<"CellPosZ[mm]"<< std::endl; // data header
  auto hashed_scoring_map = GetScoringHashedMap(run_collection,Scoring::Type::Cell);
  for(auto& scoring_volume : hashed_scoring_map){ // this is the loop over all cells in geometry layout
    auto& hit = scoring_volume.second;
    auto hit_centre = hit.GetCentre();
    outFile   << hit.GetID(0) // Cell ID X
      << sep  << hit.GetID(1) // Cell ID Y
      << sep  << hit.GetID(2) // Cell ID Z
      << sep  << hit_centre.getX() // Cell Position X
      << sep  << hit_centre.getY() // Cell Position Y
      << sep  << hit_centre.getZ() // Cell Position Z
      << std::endl;
  }
  outFile.close();
  LOGSVC_INFO("Writing Dose3D Cell Scroing Positioning to file {} - done!",file);
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
  outGateFile << "######    time [s]    rotationAngle[deg]    rotationAxisX    rotationAxisY    rotationAxisZ    CellPosX[mm]    CellPosY[mm]    CellPosZ[mm]"<< std::endl; 
  outGateFile << "Time     s\nRotation deg\nTranslation mm" << std::endl; 
  for(const auto& mLayer: m_d3d_layers){
    auto cells = mLayer->GetCells();
    for(const auto& cell: cells){
      auto label = cell->GetName();
      auto centre = cell->GetCentre();
      auto rotationAngle = cell->GetPhysicalVolume()->GetRotation();
      auto rotation = cell->GetPhysicalVolume()->GetObjectRotation();
      auto posX = centre.getX()/CLHEP::mm;
      auto posY = centre.getY()/CLHEP::mm;
      auto posZ = centre.getZ()/CLHEP::mm;
      outGateFile << 0.0 <<gateSep<< 0.0  <<gateSep<< 0 <<gateSep<< 1 <<gateSep<< 0 <<gateSep<< posX <<gateSep<< posY <<gateSep<< posZ << std::endl;
    }
  }
  outGateFile.close();
}

////////////////////////////////////////////////////////////////////////////////
///
std::map<std::size_t, VoxelHit> D3DDetector::GetScoringHashedMap(const G4String& run_collection, Scoring::Type type) const {
  if(!m_label.contains(run_collection))
    return std::map<std::size_t, VoxelHit>(); // return empty map
    
  // G4cout<<"GetScoringHashedMap for " << run_collection << " / " <<Scoring::to_string(type)<<G4endl;
  std::map<std::size_t, VoxelHit> hashed_map_scoring;
  auto size = D3DCell::SIZE;
  auto Medium = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", m_config.m_cell_medium);
  for(const auto& mLayer: m_d3d_layers){
    // TODO: hashed_map_scoring.insert(mLayer->GetScoringHashedMap(run_collection,type));
    for(const auto& cell: mLayer->GetCells()){
      auto centre = cell->GetGlobalCentre();
      auto cIdX = cell->GetIdX();
      auto cIdY = cell->GetIdY();
      auto cIdZ = cell->GetIdZ();
      auto hashedCellString = std::to_string(cIdX);
      hashedCellString+=std::to_string(cIdY);
      hashedCellString+=std::to_string(cIdZ);

      if( type==Scoring::Type::Voxel ){ 
        // !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
        // !!! By now IsAnyCellVoxelised(mLayer,run_collection) && causes 
        // std::out_of_range, wołamy o mape której nie ma...
        auto cell_sv = cell->GetSD()->GetRunCollectionReferenceScoringVolume(run_collection,true);
        if(cell_sv==nullptr) // no voxelisation for this cell, continue
          continue;
        auto nvx = cell_sv->m_nVoxelsX;
        auto nvy = cell_sv->m_nVoxelsY;
        auto nvz = cell_sv->m_nVoxelsZ;

        double pix_size_x = size / nvx;
        double pix_size_y = size / nvy;
        double pix_size_z = size / nvz;

        for(int ix=0; ix<nvx; ix++ ){
          for(int iy=0; iy<nvy; iy++ ){
            for(int iz=0; iz<nvz; iz++ ){
              auto hashedVoxelString = hashedCellString;
              hashedVoxelString+=std::to_string(ix);
              hashedVoxelString+=std::to_string(iy);
              hashedVoxelString+=std::to_string(iz);
              auto voxelHash = std::hash<std::string>{}(hashedVoxelString);
              hashed_map_scoring[voxelHash] = VoxelHit();
              auto x_centre = centre.getX() - size/2 + (ix) * pix_size_x + pix_size_x/2.;  
              auto y_centre = centre.getY() - size/2 + (iy) * pix_size_y + pix_size_y/2.;  
              auto z_centre = centre.getZ() - size/2 + (iz) * pix_size_z + pix_size_z/2.;
              hashed_map_scoring[voxelHash].SetCentre(G4ThreeVector(x_centre,y_centre,z_centre));
              hashed_map_scoring[voxelHash].SetId(ix,iy,iz);
              hashed_map_scoring[voxelHash].SetGlobalId(cIdX,cIdY,cIdZ);
              hashed_map_scoring[voxelHash].SetVolume( cell_sv->GetVoxelVolume() );
              hashed_map_scoring[voxelHash].SetMass(Medium->GetDensity()*hashed_map_scoring[voxelHash].GetVolume());
            }
          }
        }
      } else if (type==Scoring::Type::Cell){
        auto cellHash = std::hash<std::string>{}(hashedCellString);
        hashed_map_scoring[cellHash] = VoxelHit();
        hashed_map_scoring[cellHash].SetCentre(centre);
        hashed_map_scoring[cellHash].SetId(cIdX,cIdY,cIdZ);
        hashed_map_scoring[cellHash].SetGlobalId(cIdX,cIdY,cIdZ); // Id == GlobalId
        hashed_map_scoring[cellHash].SetVolume( size*size*size );
        hashed_map_scoring[cellHash].SetMass(Medium->GetDensity()*hashed_map_scoring[cellHash].GetVolume());
        // hashed_map_scoring[cellHash].Print();
      }
    }
  }
  return hashed_map_scoring;
}

////////////////////////////////////////////////////////////////////////////////
/// TODO https://jira.plgrid.pl/jira/browse/TNSIM-291
void D3DDetector::ExportLayerPads(const std::string& path_to_output_dir) const {
  // TODO: Once this method is still valid it should iterate trough all Scoring::Types
  LOGSVC_DEBUG("{} ExportLayerPads... \nOutput dir: {}", GetName(), path_to_output_dir);

    auto global_cell_id = [&](int idX, int idY, int idZ) -> int {
        return (idX * m_config.m_nY_cells + idY) * m_config.m_nZ_cells + idZ;
    };

    auto global_vox_id_in_layer = [&](int idCellX, int idCellZ, int idX, int idZ) -> int {
        return ( ( idCellX * m_config.m_nZ_cells + idCellZ) * m_config.m_cell_nX_voxels + idX) * m_config.m_cell_nZ_voxels + idZ;

    };

    auto file = path_to_output_dir+"/dose3d_alignment.root";
    auto outputFile = std::make_unique<TFile>(file.c_str(),"RECREATE");
    auto cell_dir = outputFile->mkdir("Dose3D_LayerPads");
    TDirectory* vox_dir = nullptr;
    
    auto size = D3DCell::SIZE;
    for(const auto& mLayer: m_d3d_layers){
      std::string layer = "Dose3D_MLayer_"+std::to_string(mLayer->GetId());
      // detector level (cell-like) voxelization:
      // _______________________________________
      cell_dir->cd(); // switch the current directory
      auto h2p = std::make_unique<TH2Poly>();
      h2p->SetName(layer.c_str());
      h2p->SetTitle(layer.c_str());
      auto cells = mLayer->GetCells();
      for(const auto& cell: cells){
        // Create bins:
        //     ----  (x2,y2)
        //     |  |
        //     ----
        // (x1,y1)
        auto centre = cell->GetGlobalCentre();
        auto x1 = centre.getX() - size/2.;
        auto y1 = centre.getZ() - size/2.; // Layers are placed in XZ plane!
        auto x2 = centre.getX() + size/2.;
        auto y2 = centre.getZ() + size/2.; // Layers are placed in XZ plane!
        auto binIdx = h2p->AddBin(x1,y1,x2,y2);
        auto cell_id = global_cell_id(cell->GetIdX(),cell->GetIdY(),cell->GetIdZ());
        h2p->SetBinContent(binIdx,cell_id+1);
      }
      h2p->Write();
      // cell level voxelization:
      // _______________________________________
      // Create TH2Poly for each "layer" within cell
      // Note: However, this cell level histograms defines common set for all the cells within MLayer!
      if(IsAnyCellVoxelised(mLayer,"Dose3D")){
        if(vox_dir==nullptr){ // Hide these histograms into layer level directory
          auto cell_layer = layer+"_CellVoxelisation";
          vox_dir = cell_dir->mkdir(cell_layer.c_str());
        }
        vox_dir->cd(); // switch the current directory
        // We assume that in Y-axis number of voxels si fixed within MLayer!
        std::vector<std::unique_ptr<TH2Poly>> cLayerTH2Poly;
        auto nCellLayers = cells.at(0)->GetNYVoxels();
        for(int icl=0; icl<nCellLayers;++icl){
          std::string cell_layer = layer+"_CLayer_"+std::to_string(icl);
          cLayerTH2Poly.push_back(std::make_unique<TH2Poly>());
          cLayerTH2Poly.back()->SetName(cell_layer.c_str());
          cLayerTH2Poly.back()->SetTitle(cell_layer.c_str());
        }
        // Now iterate again over cells and cell-layers and define bins
        for(int icl=0; icl<nCellLayers;++icl){
          auto h2pc = cLayerTH2Poly.at(icl).get();
          for(const auto& cell: cells){
            auto nX = cell->GetNXVoxels();
            double pix_size_x = size / nX;
            auto nY = cell->GetNYVoxels();
            double pix_size_y = size / nY;
            auto nZ = cell->GetNZVoxels();
            double pix_size_z = size / nZ;
            // Create bins for each pixel within current cell:
            //     ----  (x2,y2)
            //     |  |
            //     ----
            // (x1,y1)
            auto centre = cell->GetGlobalCentre();
            // Start in the corner of the cell
            
            for(int icX=0; icX<nX;++icX){
              for(int icZ=0; icZ<nZ;++icZ){
                auto x_centre = centre.getX() - size/2 + (icX) * pix_size_x + pix_size_x/2.;  
                auto y_centre = centre.getY() - size/2 + (icl) * pix_size_y + pix_size_y/2.;  
                auto z_centre = centre.getZ() - size/2 + (icZ) * pix_size_z + pix_size_z/2.; 
                auto x1 = x_centre - pix_size_x/2.;
                auto y1 = z_centre - pix_size_z/2.; 
                auto x2 = x_centre + pix_size_x/2.;
                auto y2 = z_centre + pix_size_z/2.;

                auto binIdx = h2pc->AddBin(x1,y1,x2,y2);
                auto vox_id = global_vox_id_in_layer(cell->GetIdX(),cell->GetIdZ(),icX,icZ);
                h2pc->SetBinContent(binIdx, vox_id+1);
              }
            }
          }
        }
        for(const auto& hist : cLayerTH2Poly)
          hist->Write();
      }
      vox_dir=nullptr; // objects are being destroyed by ROOT Obj Store
    }
    outputFile->Write();
    outputFile->Close();

}

////////////////////////////////////////////////////////////////////////////////
///

G4bool D3DDetector::IsInside(double x, double y, double z) {
  return false;
}

////////////////////////////////////////////////////////////////////////////////
///
bool D3DDetector::IsAnyCellVoxelised(D3DMLayer* layer, const G4String& run_collection) const {
  if(layer){
    for(const auto& cell: layer->GetCells()){
      if( cell->IsRunCollectionScoringVolumeVoxelised(run_collection) )
        return true; // Any cell in layer is voxelised
    }
  }
  return false;
}
////////////////////////////////////////////////////////////////////////////////
///
bool D3DDetector::IsAnyCellVoxelised(int idx, const G4String& run_collection) const {
  if (m_d3d_layers.size()>=idx)
    return false;
  return IsAnyCellVoxelised(m_d3d_layers.at(idx),run_collection);
}

////////////////////////////////////////////////////////////////////////////////
///
std::string D3DDetector::SetGeometrySource(){

  auto geo_type = "";

  std::cout << "SetGeometrySource layer csv?: " << m_config.m_in_layer_positioning_module <<std::endl;

  if((m_config.m_stl_geometry_file_path.compare("None")==0)&&(m_config.m_in_layer_positioning_module.compare("None")==0)){
    return geo_type = "Standard";
  }

  else if((m_config.m_stl_geometry_file_path.compare("None")!=0)&&(m_config.m_in_layer_positioning_module.compare("None")==0)){
    LOGSVC_ERROR("You can't build STL detector geometry without providing cell positioning in \".csv\" file format.");
    return geo_type;
  }

  else if((m_config.m_stl_geometry_file_path.compare("None")==0)&&(m_config.m_in_layer_positioning_module.compare("None")!=0)){
    ReadCellsInLayersPositioning();
    return geo_type = "PositioningFromCsv";
  }

  else if((m_config.m_stl_geometry_file_path.compare("None")!=0)&&(m_config.m_in_layer_positioning_module.compare("None")!=0)){
    ReadCellsInLayersPositioning();
    return geo_type = "StlDetectorWithPositioningFromCsv";
  }
  return geo_type;
}


////////////////////////////////////////////////////////////////////////////////
///
void D3DDetector::ReadCellsInLayersPositioning(){
  
  std::vector<G4ThreeVector> cells_in_layer;
  std::string line;
  if(m_config.m_in_layer_positioning_module.at(0)!='/'){
    std::string path = PROJECT_DATA_PATH;
    m_config.m_in_layer_positioning_module=path+"/"+m_config.m_in_layer_positioning_module;
  }
  std::ifstream file(m_config.m_in_layer_positioning_module.data());
  bool is_first_layer = true;
  if (file.is_open()) {  // check if file exists
    while (getline(file, line)) {
      if (line.length() > 0 && line.at(0) != '#') {  
        if(line.find("layer")== std::string::npos){
          std::istringstream ss(line);
          std::string svalue;
          std::vector<double> xyz;
          while (getline(ss, svalue,',')){
            xyz.emplace_back(std::strtod(svalue.c_str(),nullptr));
          }
          if(xyz.size()!=3 ){
            G4String description = "Wrong G4ThreeVector parameters number read-in from FILE!";
            G4Exception("D3DDetector", "G4ThreeVector", FatalErrorInArgument, description.data());
          }
          cells_in_layer.emplace_back(xyz.at(0),xyz.at(1),xyz.at(2));
        }
        else if(!is_first_layer) {
          LOGSVC_INFO("Adding new layer with No cells {}",cells_in_layer.size());
          m_d3d_cells_in_layers_positioning.push_back(cells_in_layer);
          cells_in_layer.clear();
        }
        is_first_layer = false;
      }
    } 
    // Add last read-in layer
    LOGSVC_INFO("Adding new layer with No cells {}",cells_in_layer.size());
    m_d3d_cells_in_layers_positioning.push_back(cells_in_layer);
    cells_in_layer.clear();
  } else {
    G4String description = "The " + m_config.m_in_layer_positioning_module + " not found";
    G4Exception("D3DDetector", "FILE", FatalErrorInArgument, description.data());
  }

}

