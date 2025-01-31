#include "PatientGeometry.hh"
#include "WaterPhantom.hh"
#include "SciSlicePhantom.hh"
#include "DishCubePhantom.hh"
#include "G4ProductionCuts.hh"
#include "D3DDetector.hh"
#include "G4SystemOfUnits.hh"
#include "Services.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "TomlConfigModule.hh"
#include "WorldConstruction.hh"
#include "IO.hh"
#include "DicomSvc.hh"
#include "IbaImRT.hh"
#include "CADMesh.hh"

namespace {
  G4Mutex phantomConstructionMutex = G4MUTEX_INITIALIZER;
}

////////////////////////////////////////////////////////////////////////////////
///
PatientGeometry::PatientGeometry()
      :IPhysicalVolume("PatientGeometry"), Configurable("PatientGeometry"){
    Configure();
  }

////////////////////////////////////////////////////////////////////////////////
///
PatientGeometry::~PatientGeometry() {
  configSvc()->Unregister(thisConfig()->GetName());
}

////////////////////////////////////////////////////////////////////////////////
///
PatientGeometry* PatientGeometry::GetInstance() {
  static PatientGeometry instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
///
void PatientGeometry::Configure() {
  G4cout << "\n\n[INFO]::  Configuring the " << thisConfig()->GetName() << G4endl;
  DefineUnit<std::string>("Type");
  DefineUnit<double>("EnviromentPositionX");
  DefineUnit<double>("EnviromentPositionY");
  DefineUnit<double>("EnviromentPositionZ");
  DefineUnit<double>("EnviromentSizeX");
  DefineUnit<double>("EnviromentSizeY");
  DefineUnit<double>("EnviromentSizeZ");
  DefineUnit<std::string>("EnviromentMedium");
  DefineUnit<std::string>("EnviromentPatientEnvelop");
  DefineUnit<std::string>("SupplementaryGeometry");
  DefineUnit<std::string>("SupplementaryGeometryMaterial");
  DefineUnit<double>("SupplementaryGeometryPositionX");
  DefineUnit<double>("SupplementaryGeometryPositionY");
  DefineUnit<double>("SupplementaryGeometryPositionZ");
  DefineUnit<std::string>("ConfigFile");
  DefineUnit<std::string>("ConfigPrefix");
  DefineUnit<double>("VoxelSizeXCT");
  DefineUnit<double>("VoxelSizeYCT");
  DefineUnit<double>("VoxelSizeZCT");

  Configurable::DefaultConfig();   // setup the default configuration for all defined units/parameters
  // G4cout << "[DEBUG]:: PatientGeometry:: Configure: DefaultConfig"<< G4endl;
  // Configurable::PrintConfig();
}

////////////////////////////////////////////////////////////////////////////////
///



void PatientGeometry::DefaultConfig(const std::string &unit) {
  // Volume name
  if (unit.compare("Label") == 0){
    thisConfig()->SetValue(unit, std::string("Patient environmet"));
    }
  if (unit.compare("Type") == 0){
    thisConfig()->SetTValue<std::string>(unit, std::string("None")); // "DishCubePhantom"  ,WaterPhantom , SciSlicePhantom, Dose3D
    // thisConfig()->SetValue(unit, std::string("Dose3D")); // "DishCubePhantom"  ,WaterPhantom , SciSlicePhantom, Dose3D
    }
  // default box size
  if (unit.compare("EnviromentPositionX") == 0){
    thisConfig()->SetTValue<double>(unit, 0.0);
    }
  if (unit.compare("EnviromentPositionY") == 0){
    thisConfig()->SetTValue<double>(unit, 0.0);
    }
  if (unit.compare("EnviromentPositionZ") == 0){
    thisConfig()->SetTValue<double>(unit, 0.0);
    }

  if (unit.compare("EnviromentSizeX") == 0){
    thisConfig()->SetTValue<double>(unit, 0.0);
    }

  if (unit.compare("EnviromentSizeY") == 0){
    thisConfig()->SetTValue<double>(unit, 0.0);
    }

  if (unit.compare("EnviromentSizeZ") == 0){
    thisConfig()->SetTValue<double>(unit, 0.0);
    }
 
 if (unit.compare("EnviromentMedium") == 0){
    thisConfig()->SetTValue<std::string>(unit, std::string("None"));
    }
  
  if (unit.compare("EnviromentPatientEnvelop")==0){
    thisConfig()->SetTValue<std::string>(unit, std::string("None"));
  }

  if (unit.compare("SupplementaryGeometryPositionX") == 0){
    thisConfig()->SetTValue<double>(unit, 0.0);
    }
  if (unit.compare("SupplementaryGeometryPositionY") == 0){
    thisConfig()->SetTValue<double>(unit, 0.0);
    }
  if (unit.compare("SupplementaryGeometryPositionZ") == 0){
    thisConfig()->SetTValue<double>(unit, 0.0);
    }
  if (unit.compare("SupplementaryGeometry")==0){
    thisConfig()->SetTValue<std::string>(unit, std::string("None"));
  }
  if (unit.compare("SupplementaryGeometryMaterial")==0){
    thisConfig()->SetTValue<std::string>(unit, std::string("None"));
  }

 if (unit.compare("ConfigFile") == 0){
    thisConfig()->SetTValue<std::string>(unit, std::string("None"));
    }
  if (unit.compare("ConfigPrefix") == 0){
    thisConfig()->SetTValue<std::string>(unit, std::string("None"));
    }
  if (unit.compare("VoxelSizeXCT") == 0){
    thisConfig()->SetTValue<double>(unit, double(1.00));
    }
  if (unit.compare("VoxelSizeYCT") == 0){
    thisConfig()->SetTValue<double>(unit, double(1.00));
    }
  if (unit.compare("VoxelSizeZCT") == 0){
    thisConfig()->SetTValue<double>(unit, double(1.00));
    }

}

////////////////////////////////////////////////////////////////////////////////
///
bool PatientGeometry::design(void) {
  auto patientType = thisConfig()->GetValue<std::string>("Type");
  G4cout << "I'm building " << patientType << "  patient geometry" << G4endl;

  if (patientType == "WaterPhantom") {
    m_patient = new WaterPhantom();
    m_patient->TomlConfig(true);
  }
  else if (patientType == "SciSlicePhantom"){
    m_patient = new SciSlicePhantom();
  }
  else if (patientType == "DishCubePhantom"){
    m_patient = new DishCubePhantom();
  }
  else if (patientType == "D3DDetector") {
    m_patient = new D3DDetector();
    m_patient->TomlConfig(true);
  }
  else 
    return false;

  // TOML-like contextual configuration
  if(m_patient->TomlConfig()){ 
    auto configFile =thisConfig()->GetValue<std::string>("ConfigFile");
    if(configFile.empty() || configFile=="None")
      m_patient->SetTomlConfigFile(); // get the job main file
    else{
      std::string projectPath = PROJECT_LOCATION_PATH;
      m_patient->SetTomlConfigFile(projectPath+configFile);
    }
    configFile = m_patient->GetTomlConfigFile();
    G4cout << "PatientGeometry::ConfigFile:: Importing configuration for \""<< patientType <<"\" from: "<< configFile << "\n" << G4endl;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
void PatientGeometry::Destroy() {
  auto pv = GetPhysicalVolume();
  if (pv) {
    if (m_patient) m_patient->Destroy();
    delete pv;
    SetPhysicalVolume(nullptr);
  }
  if (m_suplementary_volume){
    delete m_suplementary_volume;
    m_suplementary_volume = nullptr;
  }

}

////////////////////////////////////////////////////////////////////////////////
///
void PatientGeometry::Construct(G4VPhysicalVolume *parentPV) {
  PrintConfig();
  design(); // a call to select the right phantom
  auto envPatientEnvelop = thisConfig()->GetValue<std::string>("EnviromentPatientEnvelop");
  G4cout << "EnvPatientEnvelop: " << envPatientEnvelop << G4endl; 

  auto mediumName = thisConfig()->GetValue<std::string>("EnviromentMedium");
  auto medium = Service<ConfigSvc>()->GetValue<G4MaterialSPtr>("MaterialsSvc", mediumName);

  // create an envelope box filled with seleceted medium
  auto envSize = G4ThreeVector(thisConfig()->GetValue<double>("EnviromentSizeX")/2.,thisConfig()->GetValue<double>("EnviromentSizeY")/2.,thisConfig()->GetValue<double>("EnviromentSizeZ")/2.);
  G4Box *patientEnv = new G4Box("patientEnvBox", envSize.x(),envSize.y(),envSize.z());
  G4LogicalVolume *patientEnvLV = new G4LogicalVolume(patientEnv, medium.get(), "patientEnvLV", 0, 0, 0);
  // The envelope box will bo located at given point with respect to the parentPV.
  // However it shifted to Sim locatin (ie. to the positive querter of the World coordinate system)
  auto envPosX = thisConfig()->GetValue<double>("EnviromentPositionX");
  auto envPosY = thisConfig()->GetValue<double>("EnviromentPositionY");
  auto envPosZ = thisConfig()->GetValue<double>("EnviromentPositionZ");

  // Region for cuts
  auto regVol = new G4Region("phantomEnviromentRegion");
  auto cuts = new G4ProductionCuts;
  cuts->SetProductionCut(0.5 * mm);
  regVol->SetProductionCuts(cuts);
  patientEnvLV->SetRegion(regVol);
  regVol->AddRootLogicalVolume(patientEnvLV);
  SetPhysicalVolume(new G4PVPlacement(m_rotation, G4ThreeVector(envPosX,envPosY,envPosZ), "phmWorldPV", patientEnvLV, parentPV, false, 0));
  auto pv = GetPhysicalVolume();

  // create the actual phantom
  auto boxMaterial = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", "Usr_G4AIR20C"); // PMMA
  auto waterMaterial = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_WATER");

  if (envPatientEnvelop.compare("IbaImRT") == 0){
    auto ibaImRT = IbaImRT::GetInstance();
    ibaImRT->IPhysicalVolume::Construct(this,G4ThreeVector(envPosX, envPosY, envPosZ));
    m_patient->IPhysicalVolume::Construct(ibaImRT);
    m_patient->WriteInfo();
  }
  else{
    m_patient->IPhysicalVolume::Construct(this);
    m_patient->WriteInfo();
  }

  if(envPatientEnvelop.compare("ModularWaterPhantom") == 0){
    auto smallInterBox = new G4Box("smallInnerBox", 125.0*mm, 25.0*mm, 200.0*mm);
    auto smallOuterBox = new G4Box("smallOuterBox", 135.0*mm, 35.0*mm, 200.0*mm);
    auto smallAquariumBox =  new G4SubtractionSolid("smallAquaBox", smallOuterBox, smallInterBox, nullptr, G4ThreeVector(0.0*mm,0.0*mm,-10.0*mm));
    auto smallWaterFillingBox = new G4Box("smallWaterFillingBox", 125.0*mm, 25.0*mm, 165.0*mm);

    auto bigInterBox = new G4Box("bigInnerBox", 125.0*mm, 125.0*mm, 200.0*mm);
    auto bigOuterBox = new G4Box("bigOuterBox", 135.0*mm, 135.0*mm, 200.0*mm);
    auto bigAquariumBox =  new G4SubtractionSolid("bigAquaBox", bigOuterBox, bigInterBox, nullptr, G4ThreeVector(0.0*mm,0.0*mm,-10.0*mm));
    auto bigWaterFillingBox = new G4Box("bigWaterFillingBox", 125.0*mm, 125.0*mm, 165.0*mm);
    
    auto smallAquaBoxLV =  new G4LogicalVolume(smallAquariumBox, boxMaterial.get(), "smallAquaBoxLV");
    auto bigAquaBoxLV =    new G4LogicalVolume(bigAquariumBox, boxMaterial.get(), "bigAquaBoxLV");
    auto smallWaterFillingBoxLV = new G4LogicalVolume(smallWaterFillingBox, waterMaterial.get(), "smallWaterFillingBoxLV");
    auto bigWaterFillingBoxLV =   new G4LogicalVolume(bigWaterFillingBox, waterMaterial.get(), "bigWaterFillingBoxLV");
    auto pv1 =         new G4PVPlacement(m_rotation, G4ThreeVector(envPosX + 150.0*mm, envPosY, envPosZ-260.0*mm), 
                                          "smallAquaBoxPV1", smallAquaBoxLV, pv, false, 0);
    auto pv1_filling = new G4PVPlacement(m_rotation, G4ThreeVector(envPosX + 150.0*mm, envPosY, envPosZ-235.0*mm), 
                                          "smallWaterFillingBoxPV1", smallWaterFillingBoxLV, pv, false, 0);
    auto pv2 =         new G4PVPlacement(m_rotation, G4ThreeVector(envPosX - 150.0*mm, envPosY, envPosZ-260.0*mm), 
                                          "smallAquaBoxPV2", smallAquaBoxLV, pv, false, 0);
    auto pv2_filling = new G4PVPlacement(m_rotation, G4ThreeVector(envPosX - 150.0*mm, envPosY, envPosZ-235.0*mm), 
                                          "smallWaterFillingBoxPV2", smallWaterFillingBoxLV, pv, false, 0);
    auto pv3 =         new G4PVPlacement(m_rotation, G4ThreeVector(envPosX + 138.0*mm, envPosY + 173.0*mm, envPosZ-260.0*mm), 
                                          "bigAquaBoxPV3",   bigAquaBoxLV,   pv, false, 0);
    auto pv3_filling = new G4PVPlacement(m_rotation, G4ThreeVector(envPosX + 138.0*mm, envPosY + 173.0*mm, envPosZ-235.0*mm), 
                                          "bigWaterFillingBoxPV3", bigWaterFillingBoxLV, pv, false, 0);
    auto pv4 =         new G4PVPlacement(m_rotation, G4ThreeVector(envPosX - 138.0*mm, envPosY + 173.0*mm, envPosZ-260.0*mm), 
                                          "bigAquaBoxPV4",   bigAquaBoxLV,   pv, false, 0);
    auto pv4_filling = new G4PVPlacement(m_rotation, G4ThreeVector(envPosX - 138.0*mm, envPosY + 173.0*mm, envPosZ-235.0*mm), 
                                          "bigWaterFillingBoxPV4", bigWaterFillingBoxLV, pv, false, 0);
    auto pv5 =         new G4PVPlacement(m_rotation, G4ThreeVector(envPosX + 138.0*mm, envPosY - 173.0*mm, envPosZ-260.0*mm), 
                                          "bigAquaBoxPV5",   bigAquaBoxLV,   pv, false, 0);
    auto pv5_filling = new G4PVPlacement(m_rotation, G4ThreeVector(envPosX + 138.0*mm, envPosY - 173.0*mm, envPosZ-235.0*mm), 
                                          "bigWaterFillingBoxPV5", bigWaterFillingBoxLV, pv, false, 0);
    auto pv6 =         new G4PVPlacement(m_rotation, G4ThreeVector(envPosX - 138.0*mm, envPosY - 173.0*mm, envPosZ-260.0*mm), 
                                          "bigAquaBoxPV6",   bigAquaBoxLV,   pv, false, 0);
    auto pv6_filling = new G4PVPlacement(m_rotation, G4ThreeVector(envPosX - 138.0*mm, envPosY - 173.0*mm, envPosZ-235.0*mm), 
                                          "bigWaterFillingBoxPV6", bigWaterFillingBoxLV, pv, false, 0);
  }

// Creation of bed?

//  auto tableMaterial = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_POLYACRYLONITRILE");
//  auto tableHeight =  7.0*mm;
//  auto tableBox = new G4Box("TableBox", 1100.0*mm, 225.0*mm, tableHeight);
//  auto dcoverLV = new G4LogicalVolume(tableBox, tableMaterial.get(), "TableBoxLV");
//  SetPhysicalVolume(new G4PVPlacement(nullptr, G4ThreeVector(900.0,0.0,((1.0*mm)+tableHeight+envPosZ+envSize.z())), "CoverBoxPV", dcoverLV, parentPV, false, 0));
if (thisConfig()->GetValue<std::string>("SupplementaryGeometry").compare("None")!=0) {
  auto supplementaryGeometryPath = thisConfig()->GetValue<std::string>("SupplementaryGeometry");
  if (supplementaryGeometryPath.at(0)!='/'){
    std::string data_path = PROJECT_DATA_PATH;
    supplementaryGeometryPath = data_path+"/"+supplementaryGeometryPath;
  }
  auto supplementaryGeometryMaterial = thisConfig()->GetValue<std::string>("SupplementaryGeometryMaterial");
  auto suppGeoPosX = thisConfig()->GetValue<double>("SupplementaryGeometryPositionX");
  auto suppGeoPosY = thisConfig()->GetValue<double>("SupplementaryGeometryPositionY");
  auto suppGeoPosZ = thisConfig()->GetValue<double>("SupplementaryGeometryPositionZ");

  auto mesh = CADMesh::TessellatedMesh::FromSTL(supplementaryGeometryPath);
  G4VSolid* solid = mesh->GetSolid();
  auto Medium = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", supplementaryGeometryMaterial);
  auto supplementaryGeometryLV = new G4LogicalVolume(solid, Medium.get(), "LVStl_Supplementary");
  m_suplementary_volume = new G4PVPlacement(nullptr, G4ThreeVector(suppGeoPosX,suppGeoPosY,suppGeoPosZ), "PVStl_Supplementary", supplementaryGeometryLV, parentPV, false, 0);

}



}

////////////////////////////////////////////////////////////////////////////////
///
G4bool PatientGeometry::Update() {
  // TODO:: Update this GetPhysicalVolume(); then the daughter
  if (m_patient) {
    if (!m_patient->Update()) return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
void PatientGeometry::WriteInfo() {
  auto envPosX = thisConfig()->GetValue<double>("EnviromentPositionX");
  auto envPosY = thisConfig()->GetValue<double>("EnviromentPositionY");
  auto envPosZ = thisConfig()->GetValue<double>("EnviromentPositionZ");
  auto centre = G4ThreeVector(envPosX,envPosY,envPosZ);
  G4cout << "Phantom centre: " << centre / cm << " [cm] " << G4endl; 
}

////////////////////////////////////////////////////////////////////////////////
/// NOTE: This method is called from WorldConstruction::ConstructSDandField
///       which is being called in workers in MT mode
void PatientGeometry::DefineSensitiveDetector() {
  if (m_patient){
    // check if there is any analysis switched on in patient:
    auto configSvc = Service<ConfigSvc>();
    if(configSvc->GetValue<bool>("RunSvc", "StepAnalysis") ||
       configSvc->GetValue<bool>("RunSvc", "RunAnalysis") ||
       configSvc->GetValue<bool>("RunSvc", "NTupleAnalysis") ) {
      G4AutoLock lock(&phantomConstructionMutex);
      m_patient->DefineSensitiveDetector();
    } else {
      std::string worker = G4Threading::IsWorkerThread() ? "worker" : "master";
      LOGSVC_WARN("No sensitive detector defined for patient. Any analysis is switched on ({})!",worker);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
///
/**
 * @brief Exports the patient geometry to CSV files.
 * 
 * This function exports the patient geometry to CSV files for CT imaging.
 * It creates a set of CSV files for each slice of the patient and saves
 * the position and material of each voxel in the CSV format. The CSV files
 * are saved in the specified output directory.
 * 
 * @param path_to_output_dir The path to the output directory where the CSV
 *                           files will be saved.
 */
void PatientGeometry::ExportToCsvCT(const std::string& path_to_output_dir) const {
  // Get the patient environment and check if it exists
  auto patientEnv = Service<GeoSvc>()->World()->PatientEnvironment();
  if (!patientEnv) {
    return;
  }
  auto patientInstance = patientEnv->GetPatient();

  // Create a G4Navigator object
  auto g4Navigator = std::make_unique<G4Navigator>();

  // Get the world instance and set the world volume in the G4Navigator
  auto worldInstance = Service<GeoSvc>()->World();
  g4Navigator->SetWorldVolume(worldInstance->GetPhysicalVolume());
  
  // Create the output directory if it does not exist
  IO::CreateDirIfNotExits(path_to_output_dir);

  // Initialize variables
  G4String materialName;
  G4ThreeVector currentPos;

  // Get the patient position in the world environment
  auto patientPositionInWorldEnv = patientInstance->GetPatientTopPositionInWolrdEnv();

  // Get the voxel size in the x, y, and z directions
  auto sizeX = thisConfig()->GetValue<double>("VoxelSizeXCT"); 
  auto sizeY = thisConfig()->GetValue<double>("VoxelSizeYCT"); 
  auto sizeZ = thisConfig()->GetValue<double>("VoxelSizeZCT"); 

  // Get the environment size in the x, y, and z directions
  auto env_size_x = thisConfig()->GetValue<double>("EnviromentSizeX");
  auto ct_cube_init_x = -svc::round_with_prec(env_size_x/2 + thisConfig()->GetValue<double>("EnviromentPositionX") + sizeX/2.,4);

  auto env_size_y = thisConfig()->GetValue<double>("EnviromentSizeY");
  auto ct_cube_init_y = -svc::round_with_prec(env_size_y/2 + thisConfig()->GetValue<double>("EnviromentPositionY") + sizeY/2.,4);

  auto env_size_z = thisConfig()->GetValue<double>("EnviromentSizeZ");
  auto ct_cube_init_z = -svc::round_with_prec(env_size_z/2 + thisConfig()->GetValue<double>("EnviromentPositionZ") + sizeZ/2.,4);

  // Calculate the resolution in the x, y, and z directions
  G4int xResolution = env_size_x / sizeX;
  G4int yResolution = env_size_y / sizeY;
  G4int zResolution = env_size_z / sizeZ;

  // Log the resolution
  LOGSVC_INFO("ExportToCsvCT: Resolution: x {}, y {}, z {}", xResolution, yResolution, zResolution);

  // Dump metadata to file
  auto meta =  path_to_output_dir+"/../ct_series_metadata.csv";
  std::ofstream metadata_file;
  metadata_file.open(meta.c_str(), std::ios::out);

  metadata_file << "x_min," << ct_cube_init_x  << std::endl;
  metadata_file << "y_min," << ct_cube_init_y  << std::endl;
  metadata_file << "z_min," << ct_cube_init_z  << std::endl;

  metadata_file << "x_max," << svc::round_with_prec((ct_cube_init_x+env_size_x + sizeX),4) << std::endl;
  metadata_file << "y_max," << svc::round_with_prec((ct_cube_init_y+env_size_y + sizeY),4) << std::endl;
  metadata_file << "z_max," << svc::round_with_prec((ct_cube_init_z+env_size_z + sizeZ),4) << std::endl;

  metadata_file << "x_resolution," << xResolution << std::endl;
  metadata_file << "y_resolution," << yResolution << std::endl;
  metadata_file << "z_resolution," << zResolution << std::endl;

  metadata_file << "x_step," << sizeX << std::endl;
  metadata_file << "y_step," << sizeY << std::endl;
  metadata_file << "z_step," << sizeZ << std::endl;

  double source_to_isocentre = 1000;
  metadata_file << "SSD," << svc::round_with_prec((source_to_isocentre + patientPositionInWorldEnv.getZ()),4) << std::endl;

  // Iterate over each slice
  for( int y = 0; y < yResolution; y++ ){
    // Create the file name
    std::ostringstream ss;
    ss << std::setw(4) << std::setfill('0') << y+1 ;
    std::string s2(ss.str());
    auto file =  path_to_output_dir+"/img"+s2+".csv";

    // Create the CSV file
    std::string header = "X [mm],Y [mm],Z [mm],Material";
    std::ofstream c_outFile;
    c_outFile.open(file.c_str(), std::ios::out);
    c_outFile << header << std::endl;

    // Iterate over each voxel in the slice
    for( int x = 0; x < xResolution; x++ ){
      for( int z = 0; z < zResolution; z++ ){
        // Set the position of the current voxel
        currentPos.setX((ct_cube_init_x+sizeX*x));
        currentPos.setY((ct_cube_init_y+sizeY*y));
        currentPos.setZ((ct_cube_init_z+sizeZ*z));

        // Get the material of the current voxel
        materialName = g4Navigator->LocateGlobalPointAndSetup(currentPos)->GetLogicalVolume()->GetMaterial()->GetName();

        // Write the position and material to the CSV file
        c_outFile << currentPos.getX() << "," << currentPos.getY() << "," << currentPos.getZ() << "," << materialName << std::endl;
      }
    }
    c_outFile.close();
  }
}



////////////////////////////////////////////////////////////////////////////////
///
void PatientGeometry::ExportDoseToCsvCT(const G4Run* runPtr) const {
  auto patientEnv = Service<GeoSvc>()->World()->PatientEnvironment();
  if (!patientEnv) {
    return;
  }
  auto patientInstance = patientEnv->GetPatient();
    
  auto g4Navigator = std::make_unique<G4Navigator>();
  auto worldInstance = Service<GeoSvc>()->World();
  g4Navigator->SetWorldVolume(worldInstance->GetPhysicalVolume());

  auto cp = Service<RunSvc>()->CurrentControlPoint();
  auto run_id = std::to_string(runPtr->GetRunID());
  auto plan_file_name = std::filesystem::path(cp->GetPlanFile()).stem().string();
  auto path_to_output_dir = cp->GetOutputDir()+"/"+plan_file_name;
  
  IO::CreateDirIfNotExits(path_to_output_dir);

  G4String materialName;
  G4ThreeVector currentPos;

  auto patientPositionInWorldEnv = patientInstance->GetPatientTopPositionInWolrdEnv();

  auto sizeX = thisConfig()->GetValue<double>("VoxelSizeXCT"); 
  auto sizeY = thisConfig()->GetValue<double>("VoxelSizeYCT"); 
  auto sizeZ = thisConfig()->GetValue<double>("VoxelSizeZCT"); 

  auto env_size_x = thisConfig()->GetValue<double>("EnviromentSizeX");
  auto ct_cube_init_x = -svc::round_with_prec(env_size_x/2 + thisConfig()->GetValue<double>("EnviromentPositionX") + sizeX/2.,4);

  auto env_size_y = thisConfig()->GetValue<double>("EnviromentSizeY");
  auto ct_cube_init_y = -svc::round_with_prec(env_size_y/2 + thisConfig()->GetValue<double>("EnviromentPositionY") + sizeY/2.,4);

  auto env_size_z = thisConfig()->GetValue<double>("EnviromentSizeZ");
  auto ct_cube_init_z = -svc::round_with_prec(env_size_z/2 + thisConfig()->GetValue<double>("EnviromentPositionZ") + sizeZ/2.,4);


  G4int xResolution = env_size_x / sizeX;
  G4int yResolution = env_size_y / sizeY;
  G4int zResolution = env_size_z / sizeZ;

  LOGSVC_INFO("ExportDoseToCsvCT: Resolution: x {}, y {}, z {}", xResolution, yResolution, zResolution);

  // DUMP METADATA TO FILE 
  auto meta =  path_to_output_dir+"/"+plan_file_name+"_ct_dose_series_metadata.csv";
  std::ofstream metadata_file;
  metadata_file.open(meta.c_str(), std::ios::out);

  metadata_file << "x_min," << ct_cube_init_x  << std::endl;
  metadata_file << "y_min," << ct_cube_init_y  << std::endl;
  metadata_file << "z_min," << ct_cube_init_z  << std::endl;

  metadata_file << "x_max," << svc::round_with_prec((ct_cube_init_x+env_size_x),4) << std::endl;
  metadata_file << "y_max," << svc::round_with_prec((ct_cube_init_y+env_size_y),4) << std::endl;
  metadata_file << "z_max," << svc::round_with_prec((ct_cube_init_z+env_size_z),4) << std::endl;

  metadata_file << "x_resolution," << xResolution << std::endl;
  metadata_file << "y_resolution," << yResolution << std::endl;
  metadata_file << "z_resolution," << zResolution << std::endl;

  metadata_file << "x_step," << sizeX << std::endl;
  metadata_file << "y_step," << sizeY << std::endl;
  metadata_file << "z_step," << sizeZ << std::endl;

  double source_to_isocentre = 1000;
  metadata_file << "SSD," << svc::round_with_prec((source_to_isocentre + patientPositionInWorldEnv.getZ()),4) << std::endl;

  // std::vector<std::pair<double,size_t>> xMappedVoxels;
  std::vector<std::pair<double,std::pair<size_t,size_t>>> xMappedVoxels;
  std::vector<std::pair<double,std::pair<size_t,size_t>>> yMappedVoxels;
  std::vector<std::pair<double,std::pair<size_t,size_t>>> zMappedVoxels;
  std::unordered_set<std::pair<double, size_t>, pair_hash> addedVoxelsPairs;

  std::vector<std::pair<double,std::pair<size_t,size_t>>> xMappedCells;
  std::vector<std::pair<double,std::pair<size_t,size_t>>> yMappedCells;
  std::vector<std::pair<double,std::pair<size_t,size_t>>> zMappedCells;
  std::unordered_set<std::pair<double, size_t>, pair_hash> addedCellsPairs;

  /**
   * Iterate over all scoring collections and extract voxel data.
   * Create vectors of pairs to store the centre coordinates and ids
   * of voxels. Check if the pair already exists in the set and if not
   * add it to the vector.
   */
  const auto& scoring_maps = cp->GetRun()->GetScoringCollections();
  
  for(auto& scoring_map: scoring_maps){
    for(auto& scoring: scoring_map.second){
      auto scoring_type = scoring.first;
      auto data = scoring.second;
      // Check if the scoring type is voxel
      if(scoring_type==Scoring::Type::Voxel){      
        // Iterate over voxel data
        for(auto& voxel: data){
          auto& voxel_data = voxel.second;
          // Create pairs to check if the pair already exists in the set
          std::pair<double, size_t> pairToCheckX = std::make_pair(
            voxel_data.GetCentre().getX(), 
            std::hash<std::string>{}(
            "XCell" + std::to_string(voxel_data.GetGlobalID(0)) + 
            "Voxel" + std::to_string(voxel_data.GetID(0))
            ));
          std::pair<double, size_t> pairToCheckY = std::make_pair(
            voxel_data.GetCentre().getY(), std::hash<std::string>{}(
            "YCell" + std::to_string(voxel_data.GetGlobalID(1)) + 
            "Voxel" + std::to_string(voxel_data.GetID(1))));

          std::pair<double, size_t> pairToCheckZ = std::make_pair(
            voxel_data.GetCentre().getZ(), std::hash<std::string>{}(
            "ZCell" + std::to_string(voxel_data.GetGlobalID(2)) + 
            "Voxel" + std::to_string(voxel_data.GetID(2))));

          // Check if the pair already exists in the set and if not
          // add it to the vector
          if(addedVoxelsPairs.find(pairToCheckX) == addedVoxelsPairs.end()) {
              auto idsx = std::make_pair(voxel_data.GetGlobalID(0),voxel_data.GetID(0));
              xMappedVoxels.push_back(std::make_pair(voxel_data.GetCentre().getX(),idsx));
              addedVoxelsPairs.insert(pairToCheckX);            
          }
          if(addedVoxelsPairs.find(pairToCheckY) == addedVoxelsPairs.end()) {
            auto idsy = std::make_pair(voxel_data.GetGlobalID(1),voxel_data.GetID(1));
            yMappedVoxels.push_back(std::make_pair(voxel_data.GetCentre().getY(),idsy));
            addedVoxelsPairs.insert(pairToCheckY);
          }
          if(addedVoxelsPairs.find(pairToCheckZ) == addedVoxelsPairs.end()) {
            auto idsz = std::make_pair(voxel_data.GetGlobalID(2),voxel_data.GetID(2));
            zMappedVoxels.push_back(std::make_pair(voxel_data.GetCentre().getZ(),idsz));
            addedVoxelsPairs.insert(pairToCheckZ);
          }
        }
      }
      else if(scoring_type==Scoring::Type::Cell){      
        // Iterate over voxel data
        for(auto& cell: data){
          auto& cell_data = cell.second;
          // Create pairs to check if the pair already exists in the set
          std::pair<double, size_t> pairToCheckX = std::make_pair(
            cell_data.GetCentre().getX(), std::hash<std::string>{}(
            "XCell" + std::to_string(cell_data.GetGlobalID(0))));
          std::pair<double, size_t> pairToCheckY = std::make_pair(
            cell_data.GetCentre().getY(), std::hash<std::string>{}(
            "YCell" + std::to_string(cell_data.GetGlobalID(1))));
          std::pair<double, size_t> pairToCheckZ = std::make_pair(
            cell_data.GetCentre().getZ(), std::hash<std::string>{}(
            "ZCell" + std::to_string(cell_data.GetGlobalID(2))));
          // Check if the pair already exists in the set and if not
          // add it to the vector
          if(addedCellsPairs.find(pairToCheckX) == addedCellsPairs.end()) {
              auto idsx = std::make_pair(cell_data.GetGlobalID(0),cell_data.GetGlobalID(0));
              xMappedCells.push_back(std::make_pair(cell_data.GetCentre().getX(),idsx));
              addedCellsPairs.insert(pairToCheckX);            
          }
          if(addedCellsPairs.find(pairToCheckY) == addedCellsPairs.end()) {
            auto idsy = std::make_pair(cell_data.GetGlobalID(1),cell_data.GetGlobalID(1));
            yMappedCells.push_back(std::make_pair(cell_data.GetCentre().getY(),idsy));
            addedCellsPairs.insert(pairToCheckY);
          }
          if(addedCellsPairs.find(pairToCheckZ) == addedCellsPairs.end()) {
            auto idsz = std::make_pair(cell_data.GetGlobalID(2),cell_data.GetGlobalID(2));
            zMappedCells.push_back(std::make_pair(cell_data.GetCentre().getZ(),idsz));
            addedCellsPairs.insert(pairToCheckZ);
          }
        }
      }
    }
  }

    /**
     * Custom comparator function to sort a vector of pairs by the first element of the pair.
     *
     * @param a First pair to compare
     * @param b Second pair to compare
     * @return True if the first element of a is less than the first element of b, false otherwise
     */
    auto compareByFirst = [](const std::pair<double,std::pair<size_t,size_t>>& a, const std::pair<double,std::pair<size_t,size_t>>& b) {
        // Compare the first element of the pair a with the first element of the pair b
        return a.first < b.first; // Return true if a is less than b, false otherwise
    };

  std::sort(xMappedVoxels.begin(), xMappedVoxels.end(), compareByFirst);
  std::sort(yMappedVoxels.begin(), yMappedVoxels.end(), compareByFirst);
  std::sort(zMappedVoxels.begin(), zMappedVoxels.end(), compareByFirst);

  const std::map<size_t, VoxelHit>* voxelData = nullptr;
  for(auto& scoring_map: scoring_maps){
    for(auto& scoring: scoring_map.second){
      auto scoring_type = scoring.first;
      if(scoring_type==Scoring::Type::Voxel){   
        voxelData = &scoring.second;
      }
    }
  }

  std::sort(xMappedCells.begin(), xMappedCells.end(), compareByFirst);
  std::sort(yMappedCells.begin(), yMappedCells.end(), compareByFirst);
  std::sort(zMappedCells.begin(), zMappedCells.end(), compareByFirst);

  const std::map<size_t, VoxelHit>* cellData = nullptr;
  for(auto& scoring_map: scoring_maps){
    for(auto& scoring: scoring_map.second){
      auto scoring_type = scoring.first;
      if(scoring_type==Scoring::Type::Cell){   
        cellData = &scoring.second;
      }
    }
  }

  auto getVoxelHitInPosition = [&voxelData, &cellData](const G4ThreeVector& position,
                              const std::vector<std::pair<double, std::pair<size_t, size_t>>>& xVector,
                              const std::vector<std::pair<double, std::pair<size_t, size_t>>>& yVector,
                              const std::vector<std::pair<double, std::pair<size_t, size_t>>>& zVector,
                              double halfSize, Scoring::Type type) -> const VoxelHit* {
    std::pair<size_t, size_t> closestX{-1, -1};
    std::pair<size_t, size_t> closestY{-1, -1};
    std::pair<size_t, size_t> closestZ{-1, -1};
    
    double minX = xVector.front().first - halfSize;
    double minY = yVector.front().first - halfSize;
    double minZ = zVector.front().first - halfSize;
    
    double maxX = xVector.back().first + halfSize;
    double maxY = yVector.back().first + halfSize;
    double maxZ = zVector.back().first + halfSize;
    
    double minDistance = std::abs(halfSize);
    
    if (position.x() < minX || position.x() > maxX ||
        position.y() < minY || position.y() > maxY ||
        position.z() < minZ || position.z() > maxZ) {
      return nullptr;
    }
    // std::cout << "I jesteśmy w lambdzie... " << std::endl;
    for (const auto& point : xVector) {
      double distance = std::abs(position.x() - point.first);
      if (distance <= minDistance) {
        closestX = point.second;
      }
    }
    
    for (const auto& point : yVector) {
      double distance = std::abs(position.y() - point.first);
      if (distance <= minDistance) {
        closestY = point.second;
      }
    }
    
    for (const auto& point : zVector) {
      double distance = std::abs(position.z() - point.first);
      if (distance <= minDistance) {
        closestZ = point.second;
      }
    }
    
    if (closestX.first == -1 || closestY.first == -1 || closestZ.first == -1) {
      return nullptr;
    }

    if (type == Scoring::Type::Voxel) {
      return &voxelData->find(std::hash<std::string>{}(std::to_string(closestX.first)+std::to_string(closestY.first)+
                                    std::to_string(closestZ.first)+std::to_string(closestX.second)+
                                    std::to_string(closestY.second)+std::to_string(closestZ.second)))->second;}
    if (type == Scoring::Type::Cell) {
      return &cellData->find(std::hash<std::string>{}(std::to_string(closestX.first)+std::to_string(closestY.first)+
                                    std::to_string(closestZ.first)))->second;}
    
  };

  // std::cout << &voxelData <<std::endl; 
  auto c_file_merged =  path_to_output_dir+"/"+plan_file_name+"_ct_dose_cell.csv";
  auto v_file_merged =  path_to_output_dir+"/"+plan_file_name+"_ct_dose_voxel.csv";
  std::ofstream c_outFile_merged, v_outFile_merged;
  c_outFile_merged.open(c_file_merged.c_str(), std::ios::out);
  v_outFile_merged.open(v_file_merged.c_str(), std::ios::out);
  std::string header_merged = "X [mm],Y [mm],Z [mm],Id,IdX,IdY,IdZ,Material,Dose [Gy],FieldScalingFactor";
  c_outFile_merged << header_merged << std::endl;
  v_outFile_merged << header_merged << std::endl;
  std::string csv_slices_path = path_to_output_dir+"/"+plan_file_name+"_ct_dose_voxel";
  IO::CreateDirIfNotExits(csv_slices_path);
  std::string header = "X [mm],Y [mm],Z [mm],IdX,IdY,IdZ,Material,Dose [Gy],FieldScalingFactor";
  double dose = 0.;
  double fsf = 0.; // field scaling factor
  int cellIdX = 0;
  int cellIdY = 0;
  int cellIdZ = 0;
  for( int x = 0; x < xResolution; x++ ){
    dose = 0.;
    fsf = 0.;
    cellIdX = -1;
    cellIdY = -1;
    cellIdZ = -1;
    std::ostringstream ss;
    ss << std::setw(4) << std::setfill('0') << x+1 ;
    std::string s2(ss.str());
    auto file =  csv_slices_path+"/img"+s2+".csv";
    std::ofstream v_outFile;
    v_outFile.open(file.c_str(), std::ios::out);
    v_outFile << header << std::endl;
    for( int y = 0; y < yResolution; y++ ){
      for( int z = 0; z < zResolution; z++ ){
        currentPos.setX((ct_cube_init_x+sizeX*x));
        currentPos.setY((ct_cube_init_y+sizeY*y));
        currentPos.setZ((ct_cube_init_z+sizeZ*z));
        materialName = g4Navigator->LocateGlobalPointAndSetup(currentPos)->GetLogicalVolume()->GetMaterial()->GetName();
        auto materialHU = DicomSvc::GetHounsfieldScaleValue(materialName,true);
        auto voxelHit = getVoxelHitInPosition(currentPos,xMappedVoxels,yMappedVoxels,zMappedVoxels, 0.5, Scoring::Type::Voxel);
        if(voxelHit){
          dose = voxelHit->GetDose();
          fsf = voxelHit->GetFieldScalingFactor();
          cellIdX = voxelHit->GetGlobalID(0);
          cellIdY = voxelHit->GetGlobalID(1);
          cellIdZ = voxelHit->GetGlobalID(2);
        } else {
          dose = 0.;
          fsf = 0.;
          cellIdX = -1;
          cellIdY = -1;
          cellIdZ = -1;
        }
        v_outFile << currentPos.getX() << "," << currentPos.getY() << "," << currentPos.getZ() << "," << cellIdX << "," << cellIdY << "," << cellIdZ << "," << materialHU  << "," << dose << "," << fsf << std::endl;

        v_outFile_merged << currentPos.getX() << "," << currentPos.getY() << "," << currentPos.getZ() << "," << x+1 << "," << cellIdX << "," << cellIdY << "," << cellIdZ << "," << materialHU  << "," << dose << "," << fsf << std::endl;
      }
    }
    v_outFile.close();
  }
  v_outFile_merged.close();

    csv_slices_path = path_to_output_dir+"/"+plan_file_name+"_ct_dose_cell";
    IO::CreateDirIfNotExits(csv_slices_path);
    for( int x = 0; x < xResolution; x++ ){
    dose = 0.;
    fsf = 0.;
    cellIdX = -1;
    cellIdY = -1;
    cellIdZ = -1;
    std::ostringstream ss;
    ss << std::setw(4) << std::setfill('0') << x+1 ;
    std::string s2(ss.str());
    auto file =  csv_slices_path+"/img"+s2+".csv";
    std::ofstream c_outFile;
    c_outFile.open(file.c_str(), std::ios::out);
    c_outFile << header << std::endl;
    for( int y = 0; y < yResolution; y++ ){
      for( int z = 0; z < zResolution; z++ ){
        currentPos.setX((ct_cube_init_x+sizeX*x));
        currentPos.setY((ct_cube_init_y+sizeY*y));
        currentPos.setZ((ct_cube_init_z+sizeZ*z));
        materialName = g4Navigator->LocateGlobalPointAndSetup(currentPos)->GetLogicalVolume()->GetMaterial()->GetName();
        auto materialHU = DicomSvc::GetHounsfieldScaleValue(materialName,true);
        auto voxelHit = getVoxelHitInPosition(currentPos,xMappedCells,yMappedCells,zMappedCells, 5, Scoring::Type::Cell);
        if(voxelHit){
          dose = voxelHit->GetDose();
          fsf = voxelHit->GetFieldScalingFactor();
          cellIdX = voxelHit->GetGlobalID(0);
          cellIdY = voxelHit->GetGlobalID(1);
          cellIdZ = voxelHit->GetGlobalID(2);
        } else {
          dose = 0.;
          fsf = 0.;
          cellIdX = -1;
          cellIdY = -1;
          cellIdZ = -1;
        }
        c_outFile << currentPos.getX() << "," << currentPos.getY() << "," << currentPos.getZ() << "," << cellIdX << "," << cellIdY << "," << cellIdZ << "," << materialHU  << "," << dose << "," << fsf << std::endl;

        c_outFile_merged << currentPos.getX() << "," << currentPos.getY() << "," << currentPos.getZ() << "," << x+1 << "," << cellIdX << "," << cellIdY << "," << cellIdZ << "," << materialHU  << "," << dose << "," << fsf << std::endl;
      }
    }
    c_outFile.close();
  }
  c_outFile_merged.close();

}

