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
#include "GeometryBuilder.hh"
#include "GeometryDBReader.hh"
#include "ModularWaterPhantom.hh"


namespace {
  G4Mutex phantomConstructionMutex = G4MUTEX_INITIALIZER;
}

////////////////////////////////////////////////////////////////////////////////
/**
   * @brief Constructs a PatientGeometry object and initializes its configuration.
   *
   * Initializes the PatientGeometry instance, setting up configuration parameters required for patient geometry management within the simulation environment.
   */
PatientGeometry::PatientGeometry()
      :IPhysicalVolume("PatientGeometry"), Configurable("PatientGeometry"){
    Configure();
  }

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Destructor for the PatientGeometry class.
 *
 * Unregisters the configuration associated with this PatientGeometry instance.
 */
PatientGeometry::~PatientGeometry() {
  configSvc()->Unregister(thisConfig()->GetName());
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the singleton instance of the PatientGeometry class.
 *
 * Ensures that only one instance of PatientGeometry exists throughout the application.
 *
 * @return Pointer to the singleton PatientGeometry instance.
 */
PatientGeometry* PatientGeometry::GetInstance() {
  static PatientGeometry instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Defines and registers configuration parameters for patient geometry.
 *
 * Sets up configurable units for patient type, isocenter coordinates, environment size and medium, supplementary geometry, voxel sizes, and database paths. Initializes default values for all parameters.
 */
void PatientGeometry::Configure() {
  G4cout << "\n\n[INFO]::  Configuring the " << thisConfig()->GetName() << G4endl;
  DefineUnit<std::string>("Type");
  DefineUnit<double>("PatientIsocentreX");
  DefineUnit<double>("PatientIsocentreY");
  DefineUnit<double>("PatientIsocentreZ");
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
  DefineUnit<std::string>("PatientDBPath");

  Configurable::DefaultConfig();   // setup the default configuration for all defined units/parameters
  // G4cout << "[DEBUG]:: PatientGeometry:: Configure: DefaultConfig"<< G4endl;
  // Configurable::PrintConfig();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Sets the default value for a given patient geometry configuration parameter.
 *
 * Assigns a default value to the specified configuration key if it matches a recognized patient geometry parameter, such as patient type, isocenter coordinates, environment size and medium, supplementary geometry, voxel sizes, or database paths.
 *
 * @param unit The configuration parameter name for which to set the default value.
 */



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
  if (unit.compare("PatientIsocentreX") == 0){
    thisConfig()->SetTValue<double>(unit, 0.0);
    }
  if (unit.compare("PatientIsocentreY") == 0){
    thisConfig()->SetTValue<double>(unit, 0.0);
    }
  if (unit.compare("PatientIsocentreZ") == 0){
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

  if (unit.compare("DBGeometryPath")==0){
    thisConfig()->SetTValue<std::string>(unit, std::string("None"));
  }

  if (unit.compare("SupplementaryGeometry")==0){
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
  if (unit.compare("PatientDBPath") == 0){
    thisConfig()->SetTValue<std::string>(unit, std::string("None"));
    }

}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Instantiates and configures the patient phantom based on the configured patient type.
 *
 * Selects and creates the appropriate patient phantom object (e.g., WaterPhantom, SciSlicePhantom, DishCubePhantom, D3DDetector) according to the configuration. If TOML configuration is enabled, loads the configuration file for the phantom. Loads patient geometry data from the specified database path if provided.
 *
 * @return true if a recognized patient type is instantiated; false if the patient type is unknown.
 */
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

  if(thisConfig()->GetValue<std::string>("PatientDBPath") != "None"){
    auto path = std::string(PROJECT_LOCATION_PATH) + "/submodules/" + thisConfig()->GetValue<std::string>("PatientDBPath");
    GeometryDBReader::Instance().LoadDataBase(path);
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Destroys and deallocates the patient and supplementary geometry volumes.
 *
 * Deletes the patient phantom and supplementary geometry volumes if they exist, and resets the internal pointers to ensure proper cleanup of allocated resources.
 */
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
/**
 * @brief Constructs the patient environment and associated geometries within the simulation world.
 *
 * Sets up the patient envelope volume with the configured medium and dimensions, applies coordinate translations for specific envelope types, and places it relative to the parent physical volume. Instantiates and constructs the selected patient phantom and any supplementary geometries, such as a table or imported STL mesh, according to configuration parameters. Also configures production cuts and regions for the environment volume.
 *
 * @param parentPV The parent physical volume to which the patient environment and associated geometries are attached.
 */
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
  auto envPosX = thisConfig()->GetValue<double>("PatientIsocentreX");
  auto envPosY = thisConfig()->GetValue<double>("PatientIsocentreY");
  auto envPosZ = thisConfig()->GetValue<double>("PatientIsocentreZ");

  if (envPatientEnvelop.compare("IbaImRT_Full") == 0){
    IbaImRT::IbaToLocalTranslation = G4ThreeVector(90.0, -165.0, 90.0);
  } else if(envPatientEnvelop.compare("IbaImRT_Box") == 0){
    IbaImRT::IbaToLocalTranslation = G4ThreeVector(90.0, -90.0, 90.0);
  }

  // Region for cuts
  auto regVol = new G4Region("phantomEnviromentRegion");
  auto cuts = new G4ProductionCuts;
  cuts->SetProductionCut(5.0 * mm);
  regVol->SetProductionCuts(cuts);
  patientEnvLV->SetRegion(regVol);
  regVol->AddRootLogicalVolume(patientEnvLV);
  SetPhysicalVolume(new G4PVPlacement(m_rotation, G4ThreeVector(envPosX,envPosY,envPosZ)-IbaImRT::IbaToLocalTranslation, "phmWorldPV", patientEnvLV, parentPV, false, 0));
  auto pv = GetPhysicalVolume();

  if (envPatientEnvelop.compare("IbaImRT_Full") == 0 || envPatientEnvelop.compare("IbaImRT_Box") == 0){
    auto ibaImRT = IbaImRT::GetInstance();
    ibaImRT->IPhysicalVolume::Construct(this);
    m_patient->IPhysicalVolume::Construct(ibaImRT);
    m_patient->WriteInfo();
  }else if(envPatientEnvelop.compare("IbaImRT_3mf") == 0){
    auto ibaImRT = IbaImRT::GetInstance();
    ibaImRT->IPhysicalVolume::Construct(this);
    SetPhysicalVolume(pv);
    m_patient->IPhysicalVolume::Construct(this);
    m_patient->WriteInfo();
  }
  else if(envPatientEnvelop.compare("ModularWaterPhantom_simplified") == 0 || envPatientEnvelop.compare("ModularWaterPhantom_3mf") == 0){
    auto modularWaterPhantom = ModularWaterPhantom::GetInstance();
    modularWaterPhantom->SetRotation(m_rotation);
    modularWaterPhantom->IPhysicalVolume::Construct(this);
    modularWaterPhantom->WriteInfo(); 
    m_patient->IPhysicalVolume::Construct(this);
    m_patient->WriteInfo();
  }
  else{
    m_patient->IPhysicalVolume::Construct(this);
    m_patient->WriteInfo();
  }


// Creation of bed?

 auto tableMaterial = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_POLYACRYLONITRILE");
 auto tableHeight =  7.0*mm;
 auto tableBox = new G4Box("TableBox", 225.0*mm, 1100.0*mm, tableHeight);
 auto dcoverLV = new G4LogicalVolume(tableBox, tableMaterial.get(), "TableBoxLV");
 auto table = new G4PVPlacement(nullptr, G4ThreeVector(0.0,900.0,((1.0*mm)+tableHeight+envPosZ+envSize.z())), "TableBoxPV", dcoverLV, parentPV, false, 0);

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
/**
 * @brief Updates the patient phantom geometry if it exists.
 *
 * Calls the `Update()` method on the patient phantom, returning `false` if the update fails. Returns `true` if the update succeeds or if no patient phantom is present.
 *
 * @return `true` if the update is successful or no patient exists; `false` if the patient's update fails.
 */
G4bool PatientGeometry::Update() {
  // TODO:: Update this GetPhysicalVolume(); then the daughter
  if (m_patient) {
    if (!m_patient->Update()) return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Outputs the coordinates of the phantom center based on the configured isocenter.
 *
 * Prints the phantom center position in centimeters to the standard output.
 */
void PatientGeometry::WriteInfo() {
  auto envPosX = thisConfig()->GetValue<double>("PatientIsocentreX");
  auto envPosY = thisConfig()->GetValue<double>("PatientIsocentreY");
  auto envPosZ = thisConfig()->GetValue<double>("PatientIsocentreZ");
  auto centre = G4ThreeVector(envPosX,envPosY,envPosZ);
  G4cout << "Phantom centre: " << centre / cm << " [cm] " << G4endl; 
}

////////////////////////////////////////////////////////////////////////////////
/// NOTE: This method is called from WorldConstruction::ConstructSDandField
/**
 * @brief Defines the sensitive detector for the patient phantom if analysis is enabled.
 *
 * Checks if any analysis mode (step, run, or NTuple) is enabled in the configuration. If so, defines the sensitive detector for the patient phantom in a thread-safe manner. Logs a warning if no analysis is enabled and thus no sensitive detector is defined.
 */
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
      WARN_GEO("No sensitive detector defined for patient. Any analysis is switched on ({})!",worker);
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
///
/**
 * @brief Exports patient geometry voxel data to CSV files for CT imaging.
 *
 * Generates a set of CSV files, one per slice, containing the position and material name of each voxel in the patient geometry. Also writes a metadata CSV file describing the spatial bounds, resolution, and voxel step sizes. Files are saved in the specified output directory.
 *
 * @param path_to_output_dir Directory where the CSV files will be saved.
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

  // Get the voxel size in the x, y, and z directions
  auto sizeX = thisConfig()->GetValue<double>("VoxelSizeXCT"); 
  auto sizeY = thisConfig()->GetValue<double>("VoxelSizeYCT"); 
  auto sizeZ = thisConfig()->GetValue<double>("VoxelSizeZCT"); 

  // Get the environment size in the x, y, and z directions
  auto env_size_x = thisConfig()->GetValue<double>("EnviromentSizeX");
  auto ct_cube_init_x = -svc::round_with_prec(env_size_x/2 + thisConfig()->GetValue<double>("PatientIsocentreX") + sizeX/2.,4);

  auto env_size_y = thisConfig()->GetValue<double>("EnviromentSizeY");
  auto ct_cube_init_y = -svc::round_with_prec(env_size_y/2 + thisConfig()->GetValue<double>("PatientIsocentreY") + sizeY/2.,4);

  auto env_size_z = thisConfig()->GetValue<double>("EnviromentSizeZ");
  auto ct_cube_init_z = -svc::round_with_prec(env_size_z/2 + thisConfig()->GetValue<double>("PatientIsocentreZ") + sizeZ/2.,4);

  // Calculate the resolution in the x, y, and z directions
  G4int xResolution = env_size_x / sizeX;
  G4int yResolution = env_size_y / sizeY;
  G4int zResolution = env_size_z / sizeZ;

  // Log the resolution
  INFO_GEO("ExportToCsvCT: Resolution: x {}, y {}, z {}", xResolution, yResolution, zResolution);

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
  double SSD = 1000;  // TODO: calc this to be generic from any patient!
  metadata_file << "SSD," << svc::round_with_prec((SSD),4) << std::endl;

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



/**
 * @brief Exports dose data mapped to patient geometry voxels and cells as CSV files for CT analysis.
 *
 * For the current control point and run, this function generates CSV files containing spatially mapped dose, scaling factors, and material information for each voxel and cell in the patient geometry. It writes both merged and per-slice CSV files, along with metadata describing the spatial bounds, resolution, and SSD. Dose data is matched to geometry positions using a configurable tolerance, and missing data is filled with zeros. Output files are organized by plan and control point for downstream analysis.
 *
 * @param runPtr Pointer to the Geant4 run containing scoring data to be exported.
 */
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

  auto sizeX = thisConfig()->GetValue<double>("VoxelSizeXCT"); 
  auto sizeY = thisConfig()->GetValue<double>("VoxelSizeYCT"); 
  auto sizeZ = thisConfig()->GetValue<double>("VoxelSizeZCT"); 

  auto env_size_x = thisConfig()->GetValue<double>("EnviromentSizeX");
  auto ct_cube_init_x = -svc::round_with_prec(env_size_x/2 + thisConfig()->GetValue<double>("PatientIsocentreX") + sizeX/2.,4);

  auto env_size_y = thisConfig()->GetValue<double>("EnviromentSizeY");
  auto ct_cube_init_y = -svc::round_with_prec(env_size_y/2 + thisConfig()->GetValue<double>("PatientIsocentreY") + sizeY/2.,4);

  auto env_size_z = thisConfig()->GetValue<double>("EnviromentSizeZ");
  auto ct_cube_init_z = -svc::round_with_prec(env_size_z/2 + thisConfig()->GetValue<double>("PatientIsocentreZ") + sizeZ/2.,4);


  G4int xResolution = env_size_x / sizeX;
  G4int yResolution = env_size_y / sizeY;
  G4int zResolution = env_size_z / sizeZ;

  INFO_GEO("ExportDoseToCsvCT: Resolution: x {}, y {}, z {}", xResolution, yResolution, zResolution);

  // DUMP METADATA TO FILE 
  auto meta =  path_to_output_dir+"/"+plan_file_name+"_ct_dose_series_metadata.csv";
  std::ofstream metadata_file(meta.c_str(), std::ios::out);

  metadata_file << "x_min," << ct_cube_init_x  << std::endl;
  metadata_file << "y_min," << ct_cube_init_y  << std::endl;
  metadata_file << "z_min," << ct_cube_init_z  << std::endl;

  metadata_file << "x_max," << svc::round_with_prec((ct_cube_init_x+(env_size_x)),4) << std::endl;
  metadata_file << "y_max," << svc::round_with_prec((ct_cube_init_y+(env_size_y)),4) << std::endl;
  metadata_file << "z_max," << svc::round_with_prec((ct_cube_init_z+(env_size_z)),4) << std::endl;

  metadata_file << "x_resolution," << xResolution << std::endl;
  metadata_file << "y_resolution," << yResolution << std::endl;
  metadata_file << "z_resolution," << zResolution << std::endl;

  metadata_file << "x_step," << sizeX << std::endl;
  metadata_file << "y_step," << sizeY << std::endl;
  metadata_file << "z_step," << sizeZ << std::endl;

  double source_to_isocentre = 1000;
  double SSD = 1000;  // TODO: calc this to be generic from any patient!
  metadata_file << "SSD," << svc::round_with_prec((SSD),4) << std::endl;


  const auto& scoring_maps = cp->GetRun()->GetScoringCollections();


  struct MappingEntry {
      G4ThreeVector centre;
      std::array<std::pair<size_t, size_t>, 3> ids; 
      const VoxelHit* hit;
  };

  // Mapping for voxel
  std::vector<MappingEntry> voxelMappings;
  {
      std::unordered_set<std::string> voxelKeys;
      for (auto& scoring_map : scoring_maps) {
          for (auto& scoring : scoring_map.second) {
              if (scoring.first == Scoring::Type::Voxel) {
                  for (auto& voxel : scoring.second) {
                      auto& voxel_data = voxel.second;
                      // Tworzymy unikalny klucz oparty na identyfikatorach
                      std::string key = "X" + std::to_string(voxel_data.GetGlobalID(0)) + "V" + std::to_string(voxel_data.GetID(0)) +
                                        "Y" + std::to_string(voxel_data.GetGlobalID(1)) + "V" + std::to_string(voxel_data.GetID(1)) +
                                        "Z" + std::to_string(voxel_data.GetGlobalID(2)) + "V" + std::to_string(voxel_data.GetID(2));
                      if (voxelKeys.find(key) == voxelKeys.end()) {
                          voxelKeys.insert(key);
                          MappingEntry entry;
                          entry.centre = voxel_data.GetCentre();
                          entry.ids[0] = {voxel_data.GetGlobalID(0), voxel_data.GetID(0)};
                          entry.ids[1] = {voxel_data.GetGlobalID(1), voxel_data.GetID(1)};
                          entry.ids[2] = {voxel_data.GetGlobalID(2), voxel_data.GetID(2)};
                          entry.hit = &voxel_data;
                          voxelMappings.push_back(entry);
                      }
                  }
              }
          }
      }
  }

  // Mapping for cell
  std::vector<MappingEntry> cellMappings;
  {
      std::unordered_set<std::string> cellKeys;
      for (auto& scoring_map : scoring_maps) {
          for (auto& scoring : scoring_map.second) {
              if (scoring.first == Scoring::Type::Cell) {
                  for (auto& cell : scoring.second) {
                      auto& cell_data = cell.second;
                      std::string key = "X" + std::to_string(cell_data.GetGlobalID(0)) +
                                        "Y" + std::to_string(cell_data.GetGlobalID(1)) +
                                        "Z" + std::to_string(cell_data.GetGlobalID(2));
                      if (cellKeys.find(key) == cellKeys.end()) {
                          cellKeys.insert(key);
                          MappingEntry entry;
                          entry.centre = cell_data.GetCentre();
                          entry.ids[0] = {cell_data.GetGlobalID(0), cell_data.GetGlobalID(0)};
                          entry.ids[1] = {cell_data.GetGlobalID(1), cell_data.GetGlobalID(1)};
                          entry.ids[2] = {cell_data.GetGlobalID(2), cell_data.GetGlobalID(2)};
                          entry.hit = &cell_data;
                          cellMappings.push_back(entry);
                      }
                  }
              }
          }
      }
  }

  // Lambda – wyszukuje wpis (mapping) dla danej pozycji,
  // zwracając wskaźnik do VoxelHit lub nullptr, jeśli nie znaleziono pasującego obiektu.
  // tolerance odpowiada zadanej tolerancji (np. 0.5 dla voxel, 5 dla cell). (prostrza, mniej złożona lambda niż poprzednio...)
  auto getMappingHit = [&](const std::vector<MappingEntry>& mappings, const G4ThreeVector& pos, double tolerance) -> const VoxelHit* {
      const VoxelHit* bestHit = nullptr;
      double bestDist2 = std::numeric_limits<double>::max();
      for (const auto& entry : mappings) {
          double dx = entry.centre.x() - pos.x();
          double dy = entry.centre.y() - pos.y();
          double dz = entry.centre.z() - pos.z();
          // Sprawdzamy, czy różnice dla każdej osi mieszczą się w tolerancji
          if (std::abs(dx) <= tolerance && std::abs(dy) <= tolerance && std::abs(dz) <= tolerance) {
              double dist2 = dx*dx + dy*dy + dz*dz;
              if (dist2 < bestDist2) {
                  bestDist2 = dist2;
                  bestHit = entry.hit;
              }
          }
      }
      return bestHit;
  };


  auto v_file_merged = path_to_output_dir+"/"+plan_file_name+"_ct_dose_voxel.csv";
  std::ofstream v_outFile_merged(v_file_merged.c_str(), std::ios::out);
  // Control Point Meta data:
  v_outFile_merged << "# FieldArea: " + std::to_string(cp->GetRun()->GetBeamMaskArea()) << std::endl;
  auto beam_grav_centre = cp->GetRun()->GetBeamMaskeGravCentre();
  v_outFile_merged << "# FieldGravCentre: "+std::to_string(beam_grav_centre.first)+","+std::to_string (beam_grav_centre.second) << std::endl;
  std::string header_merged = "X [mm],Y [mm],Z [mm],Id,IdX,IdY,IdZ,Material,Dose [Gy],FieldScalingFactor,AngleScalingFactor";
  v_outFile_merged << header_merged << std::endl;
  std::string csv_slices_path = path_to_output_dir+"/"+plan_file_name+"_ct_dose_voxel";
  IO::CreateDirIfNotExits(csv_slices_path);
  std::string header = "X [mm],Y [mm],Z [mm],IdX,IdY,IdZ,Material,Dose [Gy],FieldScalingFactor,AngleScalingFactor";
  double dose = 0.;
  double fsf = 0.; 
  double asf = 0.; 
  int cellIdX = 0;
  int cellIdY = 0;
  int cellIdZ = 0;
  for (int x = 0; x < xResolution; x++) {
      std::ostringstream ss;
      ss << std::setw(4) << std::setfill('0') << x+1;
      std::string s2(ss.str());
      auto file = csv_slices_path + "/img" + s2 + ".csv";
      std::ofstream v_outFile(file.c_str(), std::ios::out);
      v_outFile << header << std::endl;
      for (int y = 0; y < yResolution; y++) {
          for (int z = 0; z < zResolution; z++) {
              currentPos.setX(ct_cube_init_x + sizeX * x);
              currentPos.setY(ct_cube_init_y + sizeY * y);
              currentPos.setZ(ct_cube_init_z + sizeZ * z);
              materialName = g4Navigator->LocateGlobalPointAndSetup(currentPos)->GetLogicalVolume()->GetMaterial()->GetName();
              auto materialHU = DicomSvc::GetHounsfieldScaleValue(materialName, true);
              // Szukamy voxel hit – z tolerancją 0.5 (Bo rozmiar woksela)
              const VoxelHit* voxelHit = getMappingHit(voxelMappings, currentPos, 0.5);
              if (voxelHit) {
                  dose = voxelHit->GetDose();
                  fsf = voxelHit->GetFieldScalingFactor();
                  asf = voxelHit->GetAngleScalingFactor();
                  cellIdX = voxelHit->GetGlobalID(0);
                  cellIdY = voxelHit->GetGlobalID(1);
                  cellIdZ = voxelHit->GetGlobalID(2);
              } else {
                  dose = 0.;
                  fsf = 0.;
                  asf = 0.;
                  cellIdX = -1;
                  cellIdY = -1;
                  cellIdZ = -1;
              }
              v_outFile << currentPos.getX() << "," << currentPos.getY() << "," << currentPos.getZ()
                        << "," << cellIdX << "," << cellIdY << "," << cellIdZ << "," << materialHU
                        << "," << dose << "," << fsf << "," << asf << std::endl;
              v_outFile_merged << currentPos.getX() << "," << currentPos.getY() << "," << currentPos.getZ()
                              << "," << x+1 << "," << cellIdX << "," << cellIdY << "," << cellIdZ << ","
                              << materialHU << "," << dose << "," << fsf << "," << asf << std::endl;
          }
      }
      v_outFile.close();
  }
  v_outFile_merged.close();


  auto c_file_merged = path_to_output_dir+"/"+plan_file_name+"_ct_dose_cell.csv";
  std::ofstream c_outFile_merged(c_file_merged.c_str(), std::ios::out);
  c_outFile_merged << "# FieldArea: " + std::to_string(cp->GetRun()->GetBeamMaskArea()) << std::endl;
  beam_grav_centre = cp->GetRun()->GetBeamMaskeGravCentre();
  c_outFile_merged << "# FieldGravCentre: "+std::to_string(beam_grav_centre.first)+","+std::to_string (beam_grav_centre.second) << std::endl;
  c_outFile_merged << header_merged << std::endl;
  csv_slices_path = path_to_output_dir+"/"+plan_file_name+"_ct_dose_cell";
  IO::CreateDirIfNotExits(csv_slices_path);
  for (int x = 0; x < xResolution; x++) {
      std::ostringstream ss;
      ss << std::setw(4) << std::setfill('0') << x+1;
      std::string s2(ss.str());
      auto file = csv_slices_path + "/img" + s2 + ".csv";
      std::ofstream c_outFile(file.c_str(), std::ios::out);
      c_outFile << header << std::endl;
      for (int y = 0; y < yResolution; y++) {
          for (int z = 0; z < zResolution; z++) {
              currentPos.setX(ct_cube_init_x + sizeX * x);
              currentPos.setY(ct_cube_init_y + sizeY * y);
              currentPos.setZ(ct_cube_init_z + sizeZ * z);
              materialName = g4Navigator->LocateGlobalPointAndSetup(currentPos)->GetLogicalVolume()->GetMaterial()->GetName();
              auto materialHU = DicomSvc::GetHounsfieldScaleValue(materialName, true);
              // Szukamy cell hit – z tolerancją 5
              const VoxelHit* cellHit = getMappingHit(cellMappings, currentPos, 5);
              if (cellHit) {
                  dose = cellHit->GetDose();
                  fsf = cellHit->GetFieldScalingFactor();
                  asf = cellHit->GetAngleScalingFactor();
                  cellIdX = cellHit->GetGlobalID(0);
                  cellIdY = cellHit->GetGlobalID(1);
                  cellIdZ = cellHit->GetGlobalID(2);
              } else {
                  dose = 0.;
                  fsf = 0.;
                  asf = 0.;
                  cellIdX = -1;
                  cellIdY = -1;
                  cellIdZ = -1;
              }
              c_outFile << currentPos.getX() << "," << currentPos.getY() << "," << currentPos.getZ()
                        << "," << cellIdX << "," << cellIdY << "," << cellIdZ << "," << materialHU
                        << "," << dose << "," << fsf << "," << asf << std::endl;
              c_outFile_merged << currentPos.getX() << "," << currentPos.getY() << "," << currentPos.getZ()
                              << "," << x+1 << "," << cellIdX << "," << cellIdY << "," << cellIdZ << ","
                              << materialHU << "," << dose << "," << fsf << "," << asf << std::endl;
          }
      }
      c_outFile.close();
  }
  c_outFile_merged.close();
}