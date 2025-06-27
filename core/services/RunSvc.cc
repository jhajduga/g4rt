#include "RunSvc.hh"
#include "Services.hh"
#include "UIManager.hh"
#include "ActionInitialization.hh"
#include "WorldConstruction.hh"
#include "RunAction.hh"
#include "PhysicsList.hh"
#include "G4ScoringManager.hh"
#include "G4UImanager.hh"
#include "G4PhysicalConstants.hh"
#include "CLHEP/Random/RanecuEngine.h"
#include "CLHEP/Random/RandomEngine.h" 
#include "colors.hh"
#include "G4RotationMatrix.hh"
#include "TFileMerger.h"
#ifdef G4MULTITHREADED
  #include "G4Threading.hh"
  #include "G4MTRunManager.hh"
#endif
#include "PatientGeometry.hh"
#include "D3DDetector.hh"
#include "TGeoManager.h"
#include "TGeoVolume.h"
#include "TFile.h"
#include "TTree.h"
#include <pybind11/embed.h>
#include "GeometryDBReader.hh"
#include "LinacGeometry.hh"


////////////////////////////////////////////////////////////////////////////////
///
RunSvc::RunSvc() : TomlConfigurable("RunSvc"), Logable("RunSvc"){
  std::cout << "Config start" << std::endl;
  Configure();
  std::cout << "Config end" << std::endl;
  // Instantiate and initialize all core services:
  Service<GeoSvc>();        // initialize GeoSvc
}

////////////////////////////////////////////////////////////////////////////////
///
RunSvc::~RunSvc() {
  configSvc()->Unregister(thisConfig()->GetName());
}

////////////////////////////////////////////////////////////////////////////////
///
RunSvc *RunSvc::GetInstance() {
  static RunSvc instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::RegisterRunComponent(RunComponet *element){
  m_run_components.emplace_back(element);
}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::Configure() {

  // G4cout << "[INFO]:: RunSvc :: Service default configuration " << G4endl;
  LOGSVC_INFO("Service default configuration ");
  DefineUnit<std::string>("JobName");

  // MULTI RUN
  DefineUnit<std::string>("SimConfigFile");
  DefineUnit<std::string>("LogConfigFile");
  DefineUnit<std::string>("LogConfigPrefix");


  // RUN PARAMETERS
  DefineUnit<int>("NumberOfEvents");      // Number of primary events
  DefineUnit<double>("PrintProgressFrequency");    // Fraction of total events after which progress info is printed
  DefineUnit<int>("NumberOfThreads");
#ifdef G4MULTITHREADED
  DefineUnit<int>("MaxNumberOfThreads");
#endif
  DefineUnit<long>("RNGSeed");            // Random Number Generator seed

  // PRIMARY GENERATOR
  DefineUnit<std::string>("BeamType");
  DefineUnit<double>("phspShiftZ"); 
  DefineUnit<std::string>("Physics");
  DefineUnit<bool>("EnableExtraProcesses");
  DefineUnit<double>("StepMax");
  DefineUnit<int>("idEnergy");

  // General Particle Source
  DefineUnit<std::string>("GpsMacFileName");

  // PHASE SPACE
  DefineUnit<bool>("SavePhSp");
  DefineUnit<std::string>("PhspInputFileName");
  DefineUnit<int>("PhspEvtVrtxMultiplicityTreshold");
  // DefineUnit<std::string>("PhspInputPosition");
  DefineUnit<std::string>("PhspOutputFileName");

  // ANALYSIS MANAGEMENT
  DefineUnit<bool>("RunAnalysis");
  DefineUnit<bool>("StepAnalysis");
  DefineUnit<bool>("NTupleAnalysis");
  DefineUnit<bool>("PrimariesAnalysis");
  DefineUnit<bool>("BeamAnalysis");

  DefineUnit<bool>("WriteFieldMaskToCsv");

  // DICOM OUTPUT MANAGEMENT
  DefineUnit<bool>("GenerateCT");

  DefineUnit<std::string>("OutputDir");

  Configurable::DefaultConfig();   // setup the default configuration for all defined units/parameters
  // Configurable::PrintConfig();

  MaterialsSvc::GetInstance();  // initialize/configure the materials list
}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::DefaultConfig(const std::string &unit) {
  // Multirun option
  if (unit.compare("SimConfigFile") == 0)
    thisConfig()->SetTValue<std::string>(unit, std::string("None"));

  if (unit.compare("LogConfigFile") == 0)
    thisConfig()->SetTValue<std::string>(unit, std::string("/data/config/deafult_logger.toml"));

  if (unit.compare("LogConfigPrefix") == 0)
    thisConfig()->SetTValue<std::string>(unit, std::string("Log"));

  // Config volume name
  if (unit.compare("Label") == 0) 
    thisConfig()->SetValue(unit, std::string("Geant4-RT Run Service"));

  // default simulation job name
  if (unit.compare("JobName") == 0) 
    thisConfig()->SetTValue<std::string>(unit, std::string("ExampleJobName"));

  // default number of primary events
  if (unit.compare("NumberOfEvents") == 0) 
    thisConfig()->SetTValue<int>(unit, int(1000));

  // default fraction of total events after which progress info is printed
  if (unit.compare("PrintProgressFrequency") == 0) 
    thisConfig()->SetTValue<double>(unit, 0.01);

  // default RNG Seed
  if (unit.compare("RNGSeed") == 0) 
    thisConfig()->SetValue(unit, long(137));

  // default number of CPU to be used
  if (unit.compare("NumberOfThreads") == 0) {
#ifdef G4MULTITHREADED
    thisConfig()->SetTValue<int>(unit, int(G4Threading::G4GetNumberOfCores()));
    thisConfig()->SetValue("MaxNumberOfThreads", int(G4Threading::G4GetNumberOfCores()));
#else
    thisConfig()->SetValue(unit, int(1));
#endif
  }

  // default source type
  // Available source types: phaseSpace, gps, phaseSpaceCustom
  if (unit.compare("BeamType") == 0) 
    thisConfig()->SetTValue<std::string>(unit, std::string("None")); //IAEA or gps

  if (unit.compare("PhspEvtVrtxMultiplicityTreshold") == 0){
    m_config->SetTValue<int>(unit, int(1));
  }

  // Fixed value for BeamCollimation: 1.25 cm above secondary collimator
  // if (unit.compare("phspShiftZ") == 0) thisConfig()->SetValue(unit, G4double(78.0 * cm)); // Custom phsp reader
  if (unit.compare("phspShiftZ") == 0) 
    thisConfig()->SetValue(unit, double(100 * cm)); // IAEA phsp reader

  // Default physics
  if (unit.compare("Physics") == 0) 
    thisConfig()->SetTValue<std::string>(unit, std::string("LowE_Penelope")); //      LowE_Livermore   LowE_Penelope   emstandard_opt3

  // Enable extra processes
  if (unit.compare("EnableExtraProcesses") == 0) {
    thisConfig()->SetTValue<bool>(unit, false);
  }

  // Step max
  if (unit.compare("StepMax") == 0) {
    thisConfig()->SetTValue<double>(unit, 0.0 * mm);
  }

  // default ID energy
  if (unit.compare("idEnergy") == 0) 
    thisConfig()->SetValue(unit, int(6));

  if (unit.compare("SavePhSp") == 0) 
    thisConfig()->SetValue(unit, false);

  if (unit.compare("GpsMacFileName") == 0){
    std::string data_path = PROJECT_DATA_PATH;
    thisConfig()->SetTValue<std::string>(unit, std::string(data_path+"/gps/gpsCLinac_pre.mac")); // ./gps.mac, gps_cd109_gammas_pre.mac
}

  if (unit.compare("PhspInputFileName") == 0){
    m_config->SetTValue<std::string>(unit, std::string("None"));
  }

  if (unit.compare("PhspOutputFileName") == 0) 
    thisConfig()->SetValue(unit, std::string("phasespaces"));

  if (unit.compare("DICOM") == 0) 
    thisConfig()->SetTValue<bool>(unit, false);

  if (unit.compare("RTPlanInputFile") == 0){
    std::string data_path = PROJECT_DATA_PATH;
    thisConfig()->SetValue(unit, std::string(data_path+"/dicom/example-imrt.dcm"));
  }

  if (unit.compare("RunAnalysis") == 0) 
    thisConfig()->SetTValue<bool>(unit, false);
  if (unit.compare("StepAnalysis") == 0) 
    thisConfig()->SetTValue<bool>(unit, false);
  if (unit.compare("NTupleAnalysis") == 0) 
    thisConfig()->SetTValue<bool>(unit, false);
  if (unit.compare("BeamAnalysis") == 0) 
    thisConfig()->SetTValue<bool>(unit, false);
  if (unit.compare("PrimariesAnalysis") == 0) 
    thisConfig()->SetTValue<bool>(unit, false);
  
  if (unit.compare("WriteFieldMaskToCsv") == 0) 
    thisConfig()->SetTValue<bool>(unit, false);

  if (unit.compare("GenerateCT") == 0) 
    thisConfig()->SetTValue<bool>(unit, false);

  if (unit.compare("OutputDir") == 0) 
    thisConfig()->SetTValue<std::string>(unit, std::string());

}

////////////////////////////////////////////////////////////////////////////////
///
bool RunSvc::ValidateConfig() const {
  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::Initialize(WorldConstruction* world) {
  // build a geometry
  world->Configure();
  world->Create();
  Service<GeoSvc>()->SetWorld(world);
  InitializeOutputDir();
  LogSvc::Configure();
  m_logger = LogSvc::RecreateLogger("RunSvc");
  LOGSVC_INFO("Logger recreated.");

  if (m_application_mode == OperationalMode::BuildGeometry)
    return;

  if (!m_isInitialized) {
    LOGSVC_INFO("Service initialization...");

    Configurable::ValidateConfig();
    PrintConfig();

    // Handle the context specific configuration
    auto simConfigFile =thisConfig()->GetValue<std::string>("SimConfigFile");
    if(simConfigFile.empty() || simConfigFile=="None")
      SetTomlConfigFile();
    else{
      std::string projectPath = PROJECT_LOCATION_PATH;
      SetTomlConfigFile(projectPath+simConfigFile);
    }

    // Define Control Points etc.
    DefineSimConfiguration();

    // Add simple scoring
    // if(thisConfig()->GetValue<G4String>("patientName")=="WaterPhantom"){
    //   m_macFiles.push_back("./scoring_pre.mac");   // List of commands being applied before beamOn execution
    //   m_macFiles.push_back("./scoring_post.mac");  // List of commands being applied after beamOn execution
    // }
    // TODO : when the modes of operation will be defined:
    // m_macFiles.push_back("./vis.mac");           // List of commands being applied before beamOn execution
    auto numberOfThreads = m_configSvc->GetValue<int>("RunSvc", "NumberOfThreads");
    auto physics = m_configSvc->GetValue<std::string>("RunSvc", "Physics");
    auto numberOfControlPoints = m_control_points.size();
    LOGSVC_INFO("Launching {} thread(s)",numberOfThreads);
    LOGSVC_INFO("Launching {} physics model",physics);
    LOGSVC_INFO("Launching {} control points",numberOfControlPoints);

    
 
#ifdef G4MULTITHREADED
    m_g4RunManager = new G4MTRunManager();
#else
    m_g4RunManager = new G4RunManager();
#endif
    m_isInitialized = true;
  } else {
    LOGSVC_WARN("RunSvc Service is already initialized.");
  }
  m_g4RunManager->SetRunIDCounter(1);
  
}

////////////////////////////////////////////////////////////////////////////////
///
std::string RunSvc::InitializeOutputDir() {
  auto path = m_configSvc->GetValue<std::string>("RunSvc","OutputDir");
  if(!path.empty()){ 
    svc::createDirIfNotExits(path);
    auto jobDir = RunSvc::GetJobNameLabel();
    auto nDirs = svc::countDirsInLocation(path,jobDir);
    auto jobNewDir = path+"/"+jobDir;
    if(nDirs>0) 
      jobNewDir+="_"+std::to_string(nDirs);
    svc::createDirIfNotExits(jobNewDir);
    m_configSvc->SetValue("RunSvc","OutputDir", jobNewDir);
  }
  else { // create default location for the output
    std::string jobNewDir = PROJECT_LOCATION_PATH;
    jobNewDir+="/output";
    m_configSvc->SetValue("RunSvc","OutputDir", jobNewDir);
    InitializeOutputDir();
  }
   return path;
}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::Finalize() {

  // Perform geometry exports
  WriteGeometryData();

  //
  MergeOutput(true);

  //
  auto runWorld = Service<GeoSvc>()->World();
  if(runWorld){
    runWorld->Destroy();
  }

  LOGSVC_INFO("Goodbye from G4RT!");
  LogSvc::ShutDown();

  
  py::finalize_interpreter();

}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::UserG4Initialization() {
  if (!m_isUsrG4Initialized) {
    G4Timer timer;
    timer.Start();
    LOGSVC_INFO("UserG4Initialization...");
    m_g4RunManager->SetUserInitialization(Service<GeoSvc>()->World());
    m_g4RunManager->SetUserInitialization(new PhysicsList());
    m_g4RunManager->SetUserInitialization(new ActionInitialization());

    // measure initialization time
    timer.Stop();
    LOGSVC_INFO( "Initialisation elapsed time [s]: {}", timer.GetRealElapsed());
    m_isUsrG4Initialized = true;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::Run() {
  switch (m_application_mode) {
    case OperationalMode::BuildGeometry:
      BuildGeometryMode();
      break;
    case OperationalMode::FullSimulation:
      FullSimulationMode();
      break;
    default:
      LOGSVC_ERROR("Operational mode missing!");
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::ParseTomlConfig(){

  auto criticalError = [&](const G4String& msg){
    LOGSVC_CRITICAL(msg.data());
    G4Exception("RunSvc", "ParseTomlConfig", FatalErrorInArgument, msg);
  };
  
  auto configFile = GetTomlConfigFile();
  auto configPrefix = GetTomlConfigPrefix();
  LOGSVC_INFO("Importing configuration from: {}",configFile);
  std::string configObj("Plan");
  if(!configPrefix.empty() || configPrefix=="None" ){ // It shouldn't be empty!
    configObj.insert(0,configPrefix+"_");
  } 
  else criticalError("The configuration PREFIX is not defined");

  auto config = toml::parse_file(configFile);
  
  auto _numberOfCP = config[configObj]["nControlPoints"].value_or(-1);
  // __________________________________________________________________________
  // Reading the plan from files is defined with the lowest priority
  // - parameters can be overwritten by values explicitly put in TOML file, see below
  if (config[configObj].as_table()->find("PlanInputFile")!= config[configObj].as_table()->end()){
    // Each file is assumed to define single Control Point!
    auto numberOfCP = config[configObj]["PlanInputFile"].as_array()->size();
    if (_numberOfCP>0){
      numberOfCP = _numberOfCP;
    }
    for( int i = 0; i < numberOfCP; i++ ){
      std::string planFile = config[configObj]["PlanInputFile"][i].value_or(std::string());
      if(!svc::checkIfFileExist(planFile)){
        criticalError("CP#"+std::to_string(i)+" File not found: "+planFile);
      }
      // Define the new control point configuration
      LOGSVC_INFO("Importing control point from plan file: {}",planFile);
      m_control_points_config.push_back(DicomSvc::GetControlPointConfig(i,planFile));
    }
  }
  // __________________________________________________________________________
  // Reading the plan from custom TOML inteface is defined with the highest priority
  LOGSVC_INFO("Verifying control point configuration from file: {}",configFile);
  auto n_beam_rot = config[configObj]["BeamRotation"].value_or(0.0);
  LinacGeometry::SetIsocentreDistance(config[configObj]["BeamSID"].value_or(0.0));
  if(n_beam_rot >= 0) {
    if (m_control_points_config.size()>0){ // configs already exist from plan files
      LOGSVC_INFO("Putting beam rotation to: {} degrees...",n_beam_rot); 
      for(auto& config: m_control_points_config){
        config.RotationInDeg = n_beam_rot;
      }
    }
  } else {
    G4String msg = "Beam rotation is "+std::to_string(n_beam_rot)+" but it's assumed to be >=0 degrees";
    LOGSVC_CRITICAL(msg.data());
    G4Exception("RunSvc", "BeamRotation", FatalErrorInArgument, msg);
  }

  auto n_stat = config[configObj]["nParticles"].value_or(-1);
  if(n_stat >= 0){
    if (m_control_points_config.size()>0){ // configs already exist from plan files
      LOGSVC_INFO("Putting simulation statistic to: {} particles...",n_stat); 
      for(auto& config: m_control_points_config){
        config.NEvts = n_stat;
      }
    }
  }
  if (m_control_points_config.size()>0)
    return; // we relay on configs created based on the plan files

  if (_numberOfCP>0){
    for( int i = 0; i < _numberOfCP; i++ ){
      if(n_stat<0)
        criticalError("RunSvc_Plan should include nParticles value");
      /// _______________________________________________________________________
      /// Define the new control point configuration
      m_control_points_config.emplace_back(i,n_stat,n_beam_rot);
      m_control_points_config.back().FieldType = (config[configObj]["FieldMask"][i]["Type"].value_or(std::string()));
      m_control_points_config.back().FieldSizeA = (config[configObj]["FieldMask"][i]["SizeA"].value_or(G4double(0.0)));
      m_control_points_config.back().FieldSizeB = (config[configObj]["FieldMask"][i]["SizeB"].value_or(G4double(0.0)));
      std::cout << "CP#" << i << " done! " << std::endl;
    }
  }
  else{
    criticalError("nControlPoints not found or set to zero!");
  }
}

////////////////////////////////////////////////////////////////////////////////
/// 
void RunSvc::DefineSimConfiguration(){
  if(IsTomlConfigExists()) // TODO Actually it is always true for now...
    ParseTomlConfig();
  else
    DefineSimDefaultConfig();
    
  DefineControlPoints();
}

////////////////////////////////////////////////////////////////////////////////
/// 
void RunSvc::DefineControlPoints() {
  for(const auto& icpc : m_control_points_config){
    m_control_points.emplace_back(icpc);
  }
  if(m_control_points.size()>0){
    m_current_control_point = &m_control_points.at(0);
  } else {
    G4String msg = "Any control point is created. Verify job definition";
    LOGSVC_CRITICAL(msg.data());
    G4Exception("RunSvc", "DefineControlPoints", FatalErrorInArgument, msg);
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Define simply single Control Point
void RunSvc::DefineSimDefaultConfig(){
  auto planFile = "plan/custom/rot00deg_stat1e3_3x3.dat";
  LOGSVC_INFO(" *** SETTING THE G4RUN DEFAULT CONFIGURATION *** ");
  LOGSVC_INFO(" Plan file: {}",planFile);
  m_control_points_config.push_back(DicomSvc::GetControlPointConfig(0,planFile));
}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::LoadSimulationPlan(){
  LOGSVC_INFO(" *** LOADING THE SIMULATION PLAN FOR #{} CONTROL POINT *** ",m_current_control_point->GetId());
  for(auto& rcomponent : m_run_components){
    rcomponent->SetRunConfiguration(m_current_control_point);
  }
  // Once the plan is loaded, we can fill the field mask
  m_current_control_point->FillPlanFieldMask();
}

////////////////////////////////////////////////////////////////////////////////
///
/// TODO: implement methods for exporting particular world volumes
void RunSvc::BuildGeometryMode() {
  LOGSVC_INFO("Building World Geometry...");
  m_logger->flush();
  Service<GeoSvc>()->Build();
}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::FullSimulationMode() {
  LOGSVC_INFO("FullSimulationMode");
  auto sourceName = m_configSvc->GetValue<std::string>("RunSvc", "BeamType");
  if (sourceName.compare("gps") == 0){
    auto macFile = m_configSvc->GetValue<std::string>("RunSvc", "GpsMacFileName");
    if(macFile.at(0)!='/'){ // Assuming that the path is relative to prejct data
      macFile = std::string(PROJECT_DATA_PATH)+"/"+macFile;
    }
    m_macFiles.push_back(macFile);
  }
  G4Random::setTheEngine(new CLHEP::RanecuEngine);
  auto userSeed = thisConfig()->GetValue<long>("RNGSeed");
  if (userSeed < 214) {
    G4Random::setTheSeed(userSeed);
  } else {
    long seeds[2] = {userSeed, userSeed};
    G4Random::setTheSeeds(seeds);
  }

  LOGSVC_INFO("RNG Seed: {} ", G4Random::getTheSeed()); 

  #ifdef G4MULTITHREADED
    auto NofThreads = m_configSvc->GetValue<int>("RunSvc", "NumberOfThreads");
    dynamic_cast<G4MTRunManager *>(m_g4RunManager)->SetNumberOfThreads(NofThreads);
  #endif

  UserG4Initialization();

  // Activate command-based scorer
  G4ScoringManager::GetScoringManager()->SetVerboseLevel(1);

  // Run and G4 kernel setup
  auto uiManager = UIManager::GetInstance();


  uiManager->UserRunInitialization(); // ControlPoint loop is here

  // Final stuff & cleaning
  uiManager->UserRunFinalization();

  // release world volume pointer from the G4RunManager store (otherwise it will be destroyed)
  // m_g4RunManager->SetUserInitialization(static_cast<G4VUserDetectorConstruction*>(nullptr));

  // TODO deleting runManager causes and error with closing phasespace file - figure out why ?
  // delete m_g4RunManager;
  // m_g4RunManager = nullptr;
}


////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::SetNofThreads(int val) {
  auto MaxNThresds = thisConfig()->GetValue<int>("MaxNumberOfThreads");
  m_configSvc->SetValue("RunSvc", "NumberOfThreads", val);
  if (val > MaxNThresds) {
    LOGSVC_WARN("Specified number of threads is higher than available CPUs.");
  }
}

////////////////////////////////////////////////////////////////////////////////
///
std::string RunSvc::GetJobNameLabel() {
  auto job_name = Service<ConfigSvc>()->GetValue<std::string>("RunSvc", "JobName");
  std::replace(job_name.begin(), job_name.end(), ' ', '_');
  // convert to lower case letters only
  std::transform(job_name.begin(), job_name.end(), job_name.begin(),
    [](unsigned char c){ return std::tolower(c); });
  return job_name;
}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::WriteGeometryData() const {
  LOGSVC_DEBUG("Writing Geometry Data");
  auto geoSvc = Service<GeoSvc>();
  geoSvc->WriteWorldToGdml();
  geoSvc->WriteWorldToTFile();
  geoSvc->WriteScoringComponentsPositioningToCsv();
  geoSvc->WriteScoringComponentsPositioningToTFile(); // TODO
  if(thisConfig()->GetValue<bool>("GenerateCT")){
    geoSvc->WritePatientToCsvCT();
    geoSvc->WritePatientToDicomCT();
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void RunSvc::MergeOutput(bool cleanUp) const {
  auto output_dir = thisConfig()->GetValue<std::string>("OutputDir");
  LOGSVC_INFO("Job output dir: {}",output_dir);
  auto output_file = output_dir+"/"+GetJobNameLabel()+".root";
  TFileMerger fm(kFALSE);
  fm.OutputFile(output_file.c_str());
  // __________________________________________________________________________
  // Handle .root output files - merge them all
  auto sim_dir = ControlPoint::GetOutputDir();
  auto files_to_merge = svc::getFilesInDir(sim_dir,".root");
  auto geo_dir = GeoSvc::GetOutputDir();
  auto files_to_merge_geo = svc::getFilesInDir(geo_dir,".root");
  if ((Service<ConfigSvc>()->GetValue<int>("RunSvc", "NumberOfThreads"))>0){
    auto additional_files_to_merge = svc::getFilesInDir(sim_dir+"/subjobs",".root");
    files_to_merge.insert(std::end(files_to_merge), std::begin(additional_files_to_merge), std::end(additional_files_to_merge));
  }
  files_to_merge.insert(std::end(files_to_merge), std::begin(files_to_merge_geo), std::end(files_to_merge_geo));
  for(const auto& file : files_to_merge){
    LOGSVC_DEBUG("AddFile: {}",file);
    fm.AddFile((file).c_str());
  }
  fm.Merge();
  LOGSVC_INFO("Merging to file: {} - done!",output_file);

  if(cleanUp){
    LOGSVC_INFO("Clean-up....");
    for(const auto& file : files_to_merge){
      svc::deleteFileIfExists(file);
    }
  }
}
