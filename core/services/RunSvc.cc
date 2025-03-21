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
// #include "LogSvc.hpp"

////////////////////////////////////////////////////////////////////////////////
///
/**
 * @brief Constructs a RunSvc instance.
 *
 * Initializes the simulation run by setting up configuration parameters and instantiating core services.
 * The constructor triggers the configuration process (with console output indicating its start and end)
 * and initializes the geometry service by instantiating GeoSvc.
 */
RunSvc::RunSvc() : TomlConfigurable("RunSvc"){
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
/**
 * @brief Registers simulation configuration parameters and initializes the materials service.
 *
 * This method defines a set of configuration units with associated keys and default types,
 * covering aspects such as job details, run parameters (e.g., number of events, threads, RNG seed),
 * primary generator settings, phase space options, analysis controls, and output directory.
 * After setting up these parameters via default configuration, it initializes the materials service to
 * ensure that necessary material definitions are available for the simulation run.
 */
void RunSvc::Configure() {

  // G4cout << "[INFO]:: RunSvc :: Service default configuration " << G4endl;
  // LOGSVC_INFO("Service default configuration ");
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

  // default physics
  if (unit.compare("Physics") == 0) 
    thisConfig()->SetTValue<std::string>(unit, std::string("emstandard_opt3")); //      LowE_Livermore   LowE_Penelope   emstandard_opt3

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
/**
 * @brief Initializes the simulation environment and configures the world geometry.
 *
 * This function sets up the simulation by configuring and creating the world geometry using the
 * provided WorldConstruction instance. It also establishes the output directory for simulation
 * results. In full simulation mode, if the service has not been initialized yet, it validates the
 * configuration, sets the simulation configuration (potentially from a TOML file), and creates the
 * Geant4 run manager (multithreaded if enabled). Finally, it initializes the run manager's run ID counter.
 *
 * If the application mode is set to BuildGeometry, only geometry configuration and creation is performed.
 *
 * @param world Pointer to the WorldConstruction instance used for building and configuring the simulation geometry.
 */
void RunSvc::Initialize(WorldConstruction* world) {
  // build a geometry
  world->Configure();
  world->Create();
  Service<GeoSvc>()->SetWorld(world);
  InitializeOutputDir();
  // LogSvc::Configure();
  // m_logger = LogSvc::RecreateLogger("RunSvc");
  // LOGSVC_INFO("Logger recreated.");
    // RUNSVC_INFO("Rozpoczęto działanie funkcji someModuleFunction.");
    // RUNSVC_DEBUG("Wykonuję operację wewnątrz funkcji.");
    // RUNSVC_WARNING("Ostrzeżenie! Może wystąpić błąd.");
    // RUNSVC_ERROR("Test komunikatu błędu.");

  if (m_application_mode == OperationalMode::BuildGeometry)
    return;

  if (!m_isInitialized) {
    // LOGSVC_INFO("Service initialization...");

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
    // LOGSVC_INFO("Launching {} thread(s)",numberOfThreads);
    // LOGSVC_INFO("Launching {} physics model",physics);
    // LOGSVC_INFO("Launching {} control points",numberOfControlPoints);

    
 
#ifdef G4MULTITHREADED
    m_g4RunManager = new G4MTRunManager();
#else
    m_g4RunManager = new G4RunManager();
#endif
    m_isInitialized = true;
  } else {
    // LOGSVC_WARN("RunSvc Service is already initialized.");
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
/**
 * @brief Finalizes the simulation run.
 *
 * Exports geometry data, merges output files into a consolidated file, and cleans up the world geometry if it exists.
 */
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

  // LOGSVC_INFO("Goodbye from G4RT!");
  // LogSvc::ShutDown();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Initializes user-defined components for the Geant4 simulation run.
 *
 * Configures the Geant4 run manager with the geometry, physics list, and action initialization components.
 * This setup is performed only once; subsequent calls to this method have no effect.
 */
void RunSvc::UserG4Initialization() {
  if (!m_isUsrG4Initialized) {
    G4Timer timer;
    timer.Start();
    // LOGSVC_INFO("UserG4Initialization...");
    m_g4RunManager->SetUserInitialization(Service<GeoSvc>()->World());
    m_g4RunManager->SetUserInitialization(new PhysicsList());
    m_g4RunManager->SetUserInitialization(new ActionInitialization());

    // measure initialization time
    timer.Stop();
    // LOGSVC_INFO( "Initialisation elapsed time [s]: {}", timer.GetRealElapsed());
    m_isUsrG4Initialized = true;
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Executes the simulation run.
 *
 * Logs initialization, warning, error, and debug messages to indicate the start and progress of the run.
 * The function simulates run progress by iterating through a fixed number of processing steps, then
 * dispatches to the appropriate mode-specific routine based on the configured operational mode:
 * - Calls the geometry building routine if in BuildGeometry mode.
 * - Calls the full simulation routine if in FullSimulation mode.
 * If the operational mode is unrecognized, an error message is displayed.
 */
void RunSvc::Run() {

        RUNSVC_INFO("RunSvc: Starting run.");
        RUNSVC_WARNING("RunSvc: Warning message with parameter {}", 42);
        RUNSVC_ERROR("RunSvc: Error occurred with code {}", -1);

        for (int i = 0; i < 3; ++i) {
            RUNSVC_DEBUG("RunSvc: Processing iteration {}", i);
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }

        RUNSVC_INFO("RunSvc: Finished run.");

  switch (m_application_mode) {
    case OperationalMode::BuildGeometry:
      BuildGeometryMode();
      break;
    case OperationalMode::FullSimulation:
      FullSimulationMode();
      break;
    default:
      // LOGSVC_ERROR("Operational mode missing!");
      G4cout <<"[ERROR]::Operational mode missing!"<< G4endl;
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Parses the simulation control point configuration from a TOML file.
 *
 * This method reads a TOML configuration file to extract control point settings for a simulation run.
 * It first derives the configuration object key using a prefix and then attempts to load the control
 * point definitions either from external plan files (specified under the "PlanInputFile" key) or from
 * an inline configuration. In the external plan mode, each file is verified to exist before loading its
 * control point data. In inline mode, the method validates that the number of control points and the sizes
 * of the associated arrays (for field masks, beam rotations, and particle counts) are consistent.
 * In case of missing or inconsistent configuration, a fatal error is raised.
 */
void RunSvc::ParseTomlConfig(){

  auto criticalError = [&](const G4String& msg){
    // LOGSVC_CRITICAL(msg.data());
    G4Exception("RunSvc", "ParseTomlConfig", FatalErrorInArgument, msg);
  };
  
  auto configFile = GetTomlConfigFile();
  auto configPrefix = GetTomlConfigPrefix();
  // LOGSVC_INFO("Importing configuration from: {}",configFile);
  std::string configObj("Plan");
  if(!configPrefix.empty() || configPrefix=="None" ){ // It shouldn't be empty!
    configObj.insert(0,configPrefix+"_");
  } 
  else criticalError("The configuration PREFIX is not defined");

  auto config = toml::parse_file(configFile);
  
  // __________________________________________________________________________
  // Reading the plan from files is defined with the highest priority
  if (config[configObj].as_table()->find("PlanInputFile")!= config[configObj].as_table()->end()){
    // Each file is assumed to define single Control Point!
    auto numberOfCP = config[configObj]["PlanInputFile"].as_array()->size();
    for( int i = 0; i < numberOfCP; i++ ){
      std::string planFile = config[configObj]["PlanInputFile"][i].value_or(std::string());
      if(!svc::checkIfFileExist(planFile)){
        criticalError("CP#"+std::to_string(i)+" File not found: "+planFile);
      }
      // Define the new control point configuration
      // LOGSVC_INFO("Importing control point from plan file: {}",planFile);
      m_control_points_config.push_back(DicomSvc::GetControlPointConfig(i,planFile));
    }
    return;
  }
  // __________________________________________________________________________
  // Reading the plan from custom TOML inteface is defined with the next priority
  // LOGSVC_INFO("Importing control point configuration from file: {}",configFile);
  G4double rotationInDeg = 0.;
  auto numberOfCP = config[configObj]["nControlPoints"].value_or(0);
  if(numberOfCP>0){
    auto n_fmask = config[configObj]["FieldMask"].as_array()->size();
    if(n_fmask != numberOfCP)
      criticalError("The number of field masks is not equal to the number of control points");
    auto n_beam_rot = config[configObj]["BeamRotation"].as_array()->size();
    if(n_beam_rot != numberOfCP)
      criticalError("The number of beam rotations is not equal to the number of control points");
    auto n_stat = config[configObj]["nParticles"].as_array()->size();
    if(n_stat != numberOfCP)
      criticalError("The number of particles statistics is not equal to the number of control points");

    for( int i = 0; i < numberOfCP; i++ ){
      rotationInDeg = (config[configObj]["BeamRotation"][i].value_or(0.0));
      int nEvents = config[configObj]["nParticles"][i].value_or(-1);
      if(nEvents<0)
        nEvents = thisConfig()->GetValue<int>("NumberOfEvents");
      /// _______________________________________________________________________
      /// Define the new control point configuration
      m_control_points_config.emplace_back(i,nEvents,rotationInDeg);
      m_control_points_config.back().FieldType = (config[configObj]["FieldMask"][i]["Type"].value_or(std::string()));
      m_control_points_config.back().FieldSizeA = (config[configObj]["FieldMask"][i]["SizeA"].value_or(G4double(0.0)));
      m_control_points_config.back().FieldSizeB = (config[configObj]["FieldMask"][i]["SizeB"].value_or(G4double(0.0)));
    }
  }
  else criticalError("nControlPoints not found or set to zero!");
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
/**
 * @brief Initializes simulation control points from configuration.
 *
 * Iterates through the control point configuration, adding each entry to the internal control
 * points list. If at least one control point is created, sets the current control point to the first
 * entry; otherwise, it raises a fatal exception indicating a misconfigured job definition.
 *
 * @exception G4Exception Thrown when no control points are created.
 */
void RunSvc::DefineControlPoints() {
  for(const auto& icpc : m_control_points_config){
    m_control_points.emplace_back(icpc);
  }
  if(m_control_points.size()>0){
    m_current_control_point = &m_control_points.at(0);
  } else {
    G4String msg = "Any control point is created. Verify job definition";
    // LOGSVC_CRITICAL(msg.data());
    G4Exception("RunSvc", "DefineControlPoints", FatalErrorInArgument, msg);
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Defines the default simulation control point configuration.
 *
 * This function sets up a single default control point for the simulation by using a
 * fixed plan file ("plan/custom/rot00deg_stat1e3_3x3.dat"). It retrieves the corresponding
 * control point configuration via DicomSvc::GetControlPointConfig with an index of 0, and
 * appends it to the internal control points configuration list.
 */
void RunSvc::DefineSimDefaultConfig(){
  auto planFile = "plan/custom/rot00deg_stat1e3_3x3.dat";
  // LOGSVC_INFO(" *** SETTING THE G4RUN DEFAULT CONFIGURATION *** ");
  // LOGSVC_INFO(" Plan file: {}",planFile);
  m_control_points_config.push_back(DicomSvc::GetControlPointConfig(0,planFile));
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Loads and applies the simulation plan.
 *
 * Iterates over each registered run component to set its configuration based on the current control point.
 * After updating the run components, finalizes the simulation plan by filling the control point's field mask.
 */
void RunSvc::LoadSimulationPlan(){
  // LOGSVC_INFO(" *** LOADING THE SIMULATION PLAN FOR #{} CONTROL POINT *** ",m_current_control_point->GetId());
  for(auto& rcomponent : m_run_components){
    rcomponent->SetRunConfiguration(m_current_control_point);
  }
  // Once the plan is loaded, we can fill the field mask
  m_current_control_point->FillPlanFieldMask();
}

////////////////////////////////////////////////////////////////////////////////
///
/**
 * @brief Builds the simulation's world geometry.
 *
 * Invokes the Geometry Service to construct the world geometry volumes, thereby 
 * setting up the necessary simulation environment.
 */
void RunSvc::BuildGeometryMode() {
  // LOGSVC_INFO("Building World Geometry...");
  // m_logger->flush();
  Service<GeoSvc>()->Build();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Executes the full simulation mode.
 *
 * This method sets up and runs a full simulation by:
 * - Configuring the simulation beam type and adjusting the associated macro file path when "gps" is used.
 * - Initializing the random number generator by selecting a CLHEP::RanecuEngine and setting seeds based on configuration.
 * - Configuring multi-threading settings if compiled with Geant4 multithreading support.
 * - Invoking user-defined Geant4 initialization and setting the scoring manager verbosity.
 * - Managing the simulation run lifecycle via UIManager callbacks for initialization and finalization.
 *
 * Note: Run manager clean-up is deferred due to a known issue with closing the phasespace file.
 */
void RunSvc::FullSimulationMode() {

        
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

  // LOGSVC_INFO("RNG Seed: {} ", G4Random::getTheSeed()); 

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
/**
 * @brief Sets the number of threads for the simulation run.
 *
 * Updates the configuration with the specified thread count and checks it against the maximum allowed threads.
 * If the provided value exceeds the maximum, a warning is (or would be) issued.
 *
 * @param val The desired number of threads.
 */
void RunSvc::SetNofThreads(int val) {
  auto MaxNThresds = thisConfig()->GetValue<int>("MaxNumberOfThreads");
  m_configSvc->SetValue("RunSvc", "NumberOfThreads", val);
  if (val > MaxNThresds) {
    // LOGSVC_WARN("Specified number of threads is higher than available CPUs.");
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
/**
 * @brief Exports the simulation's geometry and scoring data to external files.
 *
 * This method retrieves the geometry service and uses it to export various aspects of the simulation's setup:
 * - World geometry is written in GDML and ROOT formats.
 * - Scoring components' positioning data is exported to CSV and ROOT files.
 * - If the "GenerateCT" configuration flag is enabled, patient CT data is additionally written in CSV and DICOM CT formats.
 */
void RunSvc::WriteGeometryData() const {
  // LOGSVC_DEBUG("Writing Geometry Data");
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
/**
 * @brief Merges multiple ROOT output files into a single file.
 *
 * This method collects .root files from both the simulation and geometry output directories.
 * If multi-threading is enabled, it also includes files from subjob directories. The collected files
 * are then merged into a single ROOT file whose path is determined by the output directory and a job
 * name label. Optionally, if the cleanUp flag is true, the original individual files are deleted after merging.
 *
 * @param cleanUp When true, removes the original .root files after the merge.
 */
void RunSvc::MergeOutput(bool cleanUp) const {
  auto output_dir = thisConfig()->GetValue<std::string>("OutputDir");
  // LOGSVC_INFO("Job output dir: {}",output_dir);
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
    // LOGSVC_DEBUG("AddFile: {}",file);
    fm.AddFile((file).c_str());
  }
  fm.Merge();
  // LOGSVC_INFO("Merging to file: {} - done!",output_file);

  if(cleanUp){
    // LOGSVC_INFO("Clean-up....");
    for(const auto& file : files_to_merge){
      svc::deleteFileIfExists(file);
    }
  }
}
