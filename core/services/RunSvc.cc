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
/**
 * @brief Constructs the RunSvc singleton and initializes core services.
 *
 * Initializes the run service configuration and ensures the geometry service is instantiated.
 */
RunSvc::RunSvc() : TomlConfigurable("RunSvc"){
  std::cout << "Config start" << std::endl;
  Configure();
  std::cout << "Config end" << std::endl;
  // Instantiate and initialize all core services:
  Service<GeoSvc>();  // initialize GeoSvc
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Destructor for the RunSvc service.
 *
 * Unregisters the service configuration upon destruction.
 */
RunSvc::~RunSvc() { configSvc()->Unregister(thisConfig()->GetName()); }

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the singleton instance of the RunSvc service.
 *
 * @return Pointer to the unique RunSvc instance.
 */
RunSvc* RunSvc::GetInstance() {
  static RunSvc instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Registers a run component with the run service.
 *
 * Adds the specified run component to the internal list for configuration and execution during the simulation run.
 */
void RunSvc::RegisterRunComponent(RunComponet* element) { m_run_components.emplace_back(element); }

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Defines and initializes the configuration parameters for the run service.
 *
 * Registers all configurable units required for simulation control, including job metadata, run parameters, primary generator settings, phase space options, analysis flags, output management, and DICOM output. Sets up default values for these parameters and initializes the materials service.
 */
void RunSvc::Configure() {
  // G4cout << "[INFO]:: RunSvc :: Service default configuration " << G4endl;
  
  RUNSVC_INFO("Service default configuration ");
  DefineUnit<std::string>("JobName");

  // MULTI RUN
  DefineUnit<std::string>("SimConfigFile");
  DefineUnit<std::string>("LogConfigFile");
  DefineUnit<std::string>("LogConfigPrefix");

  // RUN PARAMETERS
  DefineUnit<int>("NumberOfEvents");             // Number of primary events
  DefineUnit<double>("PrintProgressFrequency");  // Fraction of total events after which progress info is printed
  DefineUnit<int>("NumberOfThreads");
#ifdef G4MULTITHREADED
  DefineUnit<int>("MaxNumberOfThreads");
#endif
  DefineUnit<long>("RNGSeed");  // Random Number Generator seed

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

  DefineUnit<bool>("StoreTracks");
  DefineUnit<bool>("StorePositions");
  DefineUnit<bool>("StoreEnergies");
  DefineUnit<bool>("StorePrimaries");
  DefineUnit<bool>("StoreRunInfo");
  DefineUnit<bool>("MinimalMode");

  DefineUnit<bool>("WriteFieldMaskToCsv");

  // DICOM OUTPUT MANAGEMENT
  DefineUnit<bool>("GenerateCT");

  DefineUnit<std::string>("OutputDir");

  Configurable::DefaultConfig();  // setup the default configuration for all defined units/parameters
  // Configurable::PrintConfig();

  MaterialsSvc::GetInstance();  // initialize/configure the materials list
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Sets the default value for a given configuration parameter.
 *
 * Assigns a default value to the specified configuration unit if it matches a recognized parameter name. This ensures that all supported configuration options have sensible defaults for simulation runs, including job settings, simulation parameters, physics options, analysis flags, and output management.
 *
 * @param unit The name of the configuration parameter to set to its default value.
 */
void RunSvc::DefaultConfig(const std::string& unit) {
  // Multirun option
  if (unit.compare("SimConfigFile") == 0) thisConfig()->SetTValue<std::string>(unit, std::string("None"));

  if (unit.compare("LogConfigFile") == 0) thisConfig()->SetTValue<std::string>(unit, std::string("/data/config/deafult_logger.toml"));

  if (unit.compare("LogConfigPrefix") == 0) thisConfig()->SetTValue<std::string>(unit, std::string("Log"));

  // Config volume name
  if (unit.compare("Label") == 0) thisConfig()->SetValue(unit, std::string("Geant4-RT Run Service"));

  // default simulation job name
  if (unit.compare("JobName") == 0) thisConfig()->SetTValue<std::string>(unit, std::string("ExampleJobName"));

  // default number of primary events
  if (unit.compare("NumberOfEvents") == 0) thisConfig()->SetTValue<int>(unit, int(1000));

  // default fraction of total events after which progress info is printed
  if (unit.compare("PrintProgressFrequency") == 0) thisConfig()->SetTValue<double>(unit, 0.01);

  // default RNG Seed
  if (unit.compare("RNGSeed") == 0) thisConfig()->SetValue(unit, long(137));

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
  if (unit.compare("BeamType") == 0) thisConfig()->SetTValue<std::string>(unit, std::string("None"));  // IAEA or gps

  if (unit.compare("PhspEvtVrtxMultiplicityTreshold") == 0) {
    m_config->SetTValue<int>(unit, int(1));
  }

  // Fixed value for BeamCollimation: 1.25 cm above secondary collimator
  // if (unit.compare("phspShiftZ") == 0) thisConfig()->SetValue(unit, G4double(78.0 * cm)); // Custom phsp reader
  if (unit.compare("phspShiftZ") == 0) thisConfig()->SetValue(unit, double(100 * cm));  // IAEA phsp reader

  // Default physics
  if (unit.compare("Physics") == 0) thisConfig()->SetTValue<std::string>(unit, std::string("LowE_Penelope"));  //      LowE_Livermore   LowE_Penelope   emstandard_opt3

  // Enable extra processes
  if (unit.compare("EnableExtraProcesses") == 0) {
    thisConfig()->SetTValue<bool>(unit, false);
  }

  // Step max
  if (unit.compare("StepMax") == 0) {
    thisConfig()->SetTValue<double>(unit, 0.0 * mm);
  }

  // default ID energy
  if (unit.compare("idEnergy") == 0) thisConfig()->SetValue(unit, int(6));

  if (unit.compare("SavePhSp") == 0) thisConfig()->SetValue(unit, false);

  if (unit.compare("GpsMacFileName") == 0) {
    std::string data_path = PROJECT_DATA_PATH;
    thisConfig()->SetTValue<std::string>(unit, std::string(data_path + "/gps/gpsCLinac_pre.mac"));  // ./gps.mac, gps_cd109_gammas_pre.mac
  }

  if (unit.compare("PhspInputFileName") == 0) {
    m_config->SetTValue<std::string>(unit, std::string("None"));
  }

  if (unit.compare("PhspOutputFileName") == 0) thisConfig()->SetValue(unit, std::string("phasespaces"));

  if (unit.compare("DICOM") == 0) thisConfig()->SetTValue<bool>(unit, false);

  if (unit.compare("RTPlanInputFile") == 0) {
    std::string data_path = PROJECT_DATA_PATH;
    thisConfig()->SetValue(unit, std::string(data_path + "/dicom/example-imrt.dcm"));
  }

  if (unit.compare("RunAnalysis") == 0) thisConfig()->SetTValue<bool>(unit, false);
  if (unit.compare("StepAnalysis") == 0) thisConfig()->SetTValue<bool>(unit, false);
  if (unit.compare("NTupleAnalysis") == 0) thisConfig()->SetTValue<bool>(unit, false);
  if (unit.compare("StoreTracks") == 0) thisConfig()->SetTValue<bool>(unit, false);
  if (unit.compare("StorePositions") == 0) thisConfig()->SetTValue<bool>(unit, false);
  if (unit.compare("StoreEnergies") == 0) thisConfig()->SetTValue<bool>(unit, false);
  if (unit.compare("StorePrimaries") == 0) thisConfig()->SetTValue<bool>(unit, false);
  if (unit.compare("StoreRunInfo") == 0) thisConfig()->SetTValue<bool>(unit, false);
  if (unit.compare("MinimalMode") == 0) thisConfig()->SetTValue<bool>(unit, false);
  if (unit.compare("BeamAnalysis") == 0) thisConfig()->SetTValue<bool>(unit, false);
  if (unit.compare("PrimariesAnalysis") == 0) thisConfig()->SetTValue<bool>(unit, false);

  if (unit.compare("WriteFieldMaskToCsv") == 0) thisConfig()->SetTValue<bool>(unit, false);

  if (unit.compare("GenerateCT") == 0) thisConfig()->SetTValue<bool>(unit, false);

  if (unit.compare("OutputDir") == 0) thisConfig()->SetTValue<std::string>(unit, std::string());
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Validates the current run service configuration.
 *
 * @return true Always returns true, indicating configuration validation is not enforced.
 */
bool RunSvc::ValidateConfig() const { return true; }

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Initializes the RunSvc service and prepares the simulation environment.
 *
 * Sets up the output directory, configures logging, builds and registers the world geometry, and initializes the Geant4 run manager. If not in geometry build mode, it validates and loads configuration, parses simulation plans, defines control points, and prepares the simulation for execution. Ensures initialization occurs only once per service instance.
 *
 * @param world Pointer to the WorldConstruction object representing the simulation geometry.
 */
void RunSvc::Initialize(WorldConstruction* world) {
  InitializeOutputDir();
  auto output_dir = Service<ConfigSvc>()->GetValue<std::string>("RunSvc","OutputDir");
  LogSvc::ReconfigureMainLog(output_dir + "/logs/main.log");
  LogSvc::AddModuleLogFile("RunSvc", output_dir + "/logs/RunSvc.log", loguru::Verbosity_MAX);
  // LogSvc::AddModuleLogFile("Physic", output_dir + "/logs/physic.log", loguru::Verbosity_INFO);
  // LogSvc::Configure();
  // m_logger = LogSvc::RecreateLogger("RunSvc");
  
  RUNSVC_INFO("Logger recreated.");
  // build a geometry
  world->Configure();
  world->Create();
  Service<GeoSvc>()->SetWorld(world);

  if (m_application_mode == OperationalMode::BuildGeometry) return;

  if (!m_isInitialized) {
    
  RUNSVC_INFO("Service initialization...");

    Configurable::ValidateConfig();
    PrintConfig();

    // Handle the context specific configuration
    auto simConfigFile = thisConfig()->GetValue<std::string>("SimConfigFile");
    if (simConfigFile.empty() || simConfigFile == "None")
      SetTomlConfigFile();
    else {
      std::string projectPath = PROJECT_LOCATION_PATH;
      SetTomlConfigFile(projectPath + simConfigFile);
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
    
  RUNSVC_INFO("Launching {} thread(s)", numberOfThreads);
    
  RUNSVC_INFO("Launching {} physics model", physics);
    
  RUNSVC_INFO("Launching {} control points", numberOfControlPoints);

#ifdef G4MULTITHREADED
    m_g4RunManager = new G4MTRunManager();
#else
    m_g4RunManager = new G4RunManager();
#endif
    m_isInitialized = true;
  } else {
    RUNSVC_WARNING("RunSvc Service is already initialized.");
  }
  m_g4RunManager->SetRunIDCounter(1);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Ensures the simulation output directory exists and updates the configuration with a unique job-specific subdirectory.
 *
 * If an output directory is specified in the configuration, creates it if necessary, then creates a subdirectory named after the job (with a numeric suffix if needed to avoid conflicts), and updates the configuration with the final path. If no output directory is specified, sets a default path and recursively initializes it.
 *
 * @return The original output directory path from the configuration.
 */
std::string RunSvc::InitializeOutputDir() {
  auto path = m_configSvc->GetValue<std::string>("RunSvc", "OutputDir");
  if (!path.empty()) {
    svc::createDirIfNotExits(path);
    auto jobDir = RunSvc::GetJobNameLabel();
    auto nDirs = svc::countDirsInLocation(path, jobDir);
    auto jobNewDir = path + "/" + jobDir;
    if (nDirs > 0) jobNewDir += "_" + std::to_string(nDirs);
    svc::createDirIfNotExits(jobNewDir);
    m_configSvc->SetValue("RunSvc", "OutputDir", jobNewDir);
  } else {  // create default location for the output
    std::string jobNewDir = PROJECT_LOCATION_PATH;
    jobNewDir += "/output";
    m_configSvc->SetValue("RunSvc", "OutputDir", jobNewDir);
    InitializeOutputDir();
  }
  return path;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Finalizes the simulation run by exporting geometry data, merging output files, and cleaning up resources.
 *
 * This method writes geometry data to output files, merges simulation output files, destroys the world geometry, and logs the shutdown message.
 */
void RunSvc::Finalize() {
  // Perform geometry exports
  WriteGeometryData();

  //
  MergeOutput(true);

  //
  auto runWorld = Service<GeoSvc>()->World();
  if (runWorld) {
    runWorld->Destroy();
  }

  
  RUNSVC_INFO("Goodbye from G4RT!");
  // LogSvc::ShutDown();

  // py::finalize_interpreter();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Performs one-time Geant4 user initialization for the simulation run.
 *
 * Sets up the user detector construction, physics list, and action initialization in the Geant4 run manager. Measures and logs the initialization time. Ensures initialization occurs only once per run.
 */
void RunSvc::UserG4Initialization() {
  if (!m_isUsrG4Initialized) {
    G4Timer timer;
    timer.Start();
    
  RUNSVC_INFO("UserG4Initialization...");
    m_g4RunManager->SetUserInitialization(Service<GeoSvc>()->World());
    m_g4RunManager->SetUserInitialization(new PhysicsList());
    m_g4RunManager->SetUserInitialization(new ActionInitialization());

    // measure initialization time
    timer.Stop();
    
  RUNSVC_INFO("Initialisation elapsed time [s]: {}", timer.GetRealElapsed());
    m_isUsrG4Initialized = true;
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Executes the simulation run according to the selected operational mode.
 *
 * Runs either the geometry building mode or the full simulation mode based on the current configuration. Logs an error if the operational mode is not set.
 */
void RunSvc::Run() {
  switch (m_application_mode) {
    case OperationalMode::BuildGeometry:
      BuildGeometryMode();
      break;
    case OperationalMode::FullSimulation:
      FullSimulationMode();
      break;
    default:
      RUNSVC_ERROR("Operational mode missing!");
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Parses the TOML configuration file to define simulation control points.
 *
 * Reads simulation parameters and control point definitions from a TOML file, supporting both plan file-based and direct TOML-based control point specification. Updates internal control point configurations with beam rotation and particle count if provided. Throws a fatal Geant4 exception if required parameters or files are missing.
 */
void RunSvc::ParseTomlConfig() {
  auto criticalError = [&](const G4String& msg) {
    RUNSVC_FATAL(msg.data());
    G4Exception("RunSvc", "ParseTomlConfig", FatalErrorInArgument, msg);
  };

  auto configFile = GetTomlConfigFile();
  auto configPrefix = GetTomlConfigPrefix();
  
  RUNSVC_INFO("Importing configuration from: {}", configFile);
  std::string configObj("Plan");
  if (!configPrefix.empty() || configPrefix == "None") {  // It shouldn't be empty!
    configObj.insert(0, configPrefix + "_");
  } else
    criticalError("The configuration PREFIX is not defined");

  auto config = toml::parse_file(configFile);

  auto _numberOfCP = config[configObj]["nControlPoints"].value_or(-1);
  // __________________________________________________________________________
  // Reading the plan from files is defined with the lowest priority
  // - parameters can be overwritten by values explicitly put in TOML file, see below
  if (config[configObj].as_table()->find("PlanInputFile") != config[configObj].as_table()->end()) {
    // Each file is assumed to define single Control Point!
    auto numberOfCP = config[configObj]["PlanInputFile"].as_array()->size();
    if (_numberOfCP > 0) {
      numberOfCP = _numberOfCP;
    }
    for (int i = 0; i < numberOfCP; i++) {
      std::string planFile = config[configObj]["PlanInputFile"][i].value_or(std::string());
      if (!svc::checkIfFileExist(planFile)) {
        criticalError("CP#" + std::to_string(i) + " File not found: " + planFile);
      }
      // Define the new control point configuration
      
  RUNSVC_INFO("Importing control point from plan file: {}", planFile);
      m_control_points_config.push_back(DicomSvc::GetControlPointConfig(i, planFile));
    }
  }
  // __________________________________________________________________________
  // Reading the plan from custom TOML inteface is defined with the highest priority
  
  RUNSVC_INFO("Verifying control point configuration from file: {}", configFile);
  auto n_beam_rot = config[configObj]["BeamRotation"].value_or(0.0);
  LinacGeometry::SetIsocentreDistance(config[configObj]["BeamSID"].value_or(0.0));
  if (n_beam_rot >= 0) {
    if (m_control_points_config.size() > 0) {  // configs already exist from plan files
      
  RUNSVC_INFO("Putting beam rotation to: {} degrees...", n_beam_rot);
      for (auto& config : m_control_points_config) {
        config.RotationInDeg = n_beam_rot;
      }
    }
  } else {
    G4String msg = "Beam rotation is " + std::to_string(n_beam_rot) + " but it's assumed to be >=0 degrees";
    RUNSVC_FATAL(msg.data());
    G4Exception("RunSvc", "BeamRotation", FatalErrorInArgument, msg);
  }

  auto n_stat = config[configObj]["nParticles"].value_or(-1);
  if (n_stat >= 0) {
    if (m_control_points_config.size() > 0) {  // configs already exist from plan files
      
  RUNSVC_INFO("Putting simulation statistic to: {} particles...", n_stat);
      for (auto& config : m_control_points_config) {
        config.NEvts = n_stat;
      }
    }
  }
  if (m_control_points_config.size() > 0) return;  // we relay on configs created based on the plan files

  if (_numberOfCP > 0) {
    for (int i = 0; i < _numberOfCP; i++) {
      if (n_stat < 0) criticalError("RunSvc_Plan should include nParticles value");
      /// _______________________________________________________________________
      /// Define the new control point configuration
      m_control_points_config.emplace_back(i, n_stat, n_beam_rot);
      m_control_points_config.back().FieldType = (config[configObj]["FieldMask"][i]["Type"].value_or(std::string()));
      m_control_points_config.back().FieldSizeA = (config[configObj]["FieldMask"][i]["SizeA"].value_or(G4double(0.0)));
      m_control_points_config.back().FieldSizeB = (config[configObj]["FieldMask"][i]["SizeB"].value_or(G4double(0.0)));
      std::cout << "CP#" << i << " done! " << std::endl;
    }
  } else {
    criticalError("nControlPoints not found or set to zero!");
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Defines the simulation configuration and control points.
 *
 * Loads simulation configuration from a TOML file if available; otherwise, sets a default configuration. After configuration is established, defines the simulation control points.
 */
void RunSvc::DefineSimConfiguration() {
  if (IsTomlConfigExists())  // TODO Actually it is always true for now...
    ParseTomlConfig();
  else
    DefineSimDefaultConfig();

  DefineControlPoints();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Initializes the list of simulation control points from the configuration.
 *
 * Copies control point configurations into the internal control points vector and sets the current control point pointer. Throws a fatal exception if no control points are defined.
 */
void RunSvc::DefineControlPoints() {
  for (const auto& icpc : m_control_points_config) {
    m_control_points.emplace_back(icpc);
  }
  if (m_control_points.size() > 0) {
    m_current_control_point = &m_control_points.at(0);
  } else {
    G4String msg = "Any control point is created. Verify job definition";
    RUNSVC_FATAL(msg.data());
    G4Exception("RunSvc", "DefineControlPoints", FatalErrorInArgument, msg);
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Defines a default simulation configuration with a single control point.
 *
 * Sets up a default control point using a hardcoded plan file and adds it to the control points configuration.
 */
void RunSvc::DefineSimDefaultConfig() {
  auto planFile = "plan/custom/rot00deg_stat1e3_3x3.dat";
  RUNSVC_INFO(" *** SETTING THE G4RUN DEFAULT CONFIGURATION *** ");
  RUNSVC_INFO(" Plan file: {}", planFile);
  m_control_points_config.push_back(DicomSvc::GetControlPointConfig(0, planFile));
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Loads the simulation plan for the current control point.
 *
 * Updates all registered run components with the current control point's configuration and fills the plan field mask.
 */
void RunSvc::LoadSimulationPlan() {
  
  RUNSVC_INFO(" *** LOADING THE SIMULATION PLAN FOR #{} CONTROL POINT *** ", m_current_control_point->GetId());
  for (auto& rcomponent : m_run_components) {
    rcomponent->SetRunConfiguration(m_current_control_point);
  }
  // Once the plan is loaded, we can fill the field mask
  m_current_control_point->FillPlanFieldMask();
}

////////////////////////////////////////////////////////////////////////////////
///
/**
 * @brief Executes the geometry building mode by constructing the world geometry.
 *
 * Calls the geometry service to build the simulation world geometry. Intended for use when only geometry construction is required, without running a full simulation.
 */
void RunSvc::BuildGeometryMode() {
  
  RUNSVC_INFO("Building World Geometry...");
  // m_logger->flush();
  Service<GeoSvc>()->Build();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Executes the full simulation workflow for the current run configuration.
 *
 * Sets up the primary particle source macro if required, initializes the random number generator with the configured seed, configures multithreading if enabled, performs Geant4 user initialization, activates the scoring manager, and runs the simulation via the UI manager. Handles all necessary preparation and finalization steps for a complete simulation run.
 */
void RunSvc::FullSimulationMode() {
  
  RUNSVC_INFO("FullSimulationMode");
  auto sourceName = m_configSvc->GetValue<std::string>("RunSvc", "BeamType");
  if (sourceName.compare("gps") == 0) {
    auto macFile = m_configSvc->GetValue<std::string>("RunSvc", "GpsMacFileName");
    if (macFile.at(0) != '/') {  // Assuming that the path is relative to prejct data
      macFile = std::string(PROJECT_DATA_PATH) + "/" + macFile;
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

  
  RUNSVC_INFO("RNG Seed: {} ", G4Random::getTheSeed());

#ifdef G4MULTITHREADED
  auto NofThreads = m_configSvc->GetValue<int>("RunSvc", "NumberOfThreads");
  dynamic_cast<G4MTRunManager*>(m_g4RunManager)->SetNumberOfThreads(NofThreads);
#endif

  UserG4Initialization();

  // Activate command-based scorer
  G4ScoringManager::GetScoringManager()->SetVerboseLevel(1);

  // Run and G4 kernel setup
  auto uiManager = UIManager::GetInstance();

  uiManager->UserRunInitialization();  // ControlPoint loop is here

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
 * Updates the configuration with the requested number of threads and warns if the value exceeds the maximum available CPUs.
 *
 * @param val Desired number of threads to use for the simulation.
 */
void RunSvc::SetNofThreads(int val) {
  auto MaxNThresds = thisConfig()->GetValue<int>("MaxNumberOfThreads");
  m_configSvc->SetValue("RunSvc", "NumberOfThreads", val);
  if (val > MaxNThresds) {
    RUNSVC_WARNING("Specified number of threads is higher than available CPUs.");
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the job name label formatted for use in file and directory names.
 *
 * Retrieves the job name from the configuration, replaces spaces with underscores, and converts all characters to lowercase.
 *
 * @return std::string The sanitized job name label.
 */
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
 * @brief Exports geometry and scoring component data to various output formats.
 *
 * Writes the world geometry to GDML and ROOT files, exports scoring component positions to CSV and ROOT files, and, if enabled, generates patient CT data in CSV and DICOM formats.
 */
void RunSvc::WriteGeometryData() const {

  RUNSVC_DEBUG("Writing Geometry Data");
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
 * @brief Merges all ROOT output files from simulation and geometry directories into a single output file.
 *
 * Collects ROOT files from the simulation and geometry output directories, including subjob files if multithreading is enabled, and merges them into a single ROOT file named after the job. Optionally deletes the original files after merging if `cleanUp` is true.
 *
 * @param cleanUp If true, deletes the individual ROOT files after merging.
 */
void RunSvc::MergeOutput(bool cleanUp) const {
  auto output_dir = thisConfig()->GetValue<std::string>("OutputDir");
  
  RUNSVC_INFO("Job output dir: {}",output_dir);
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

    RUNSVC_DEBUG("AddFile: {}",file);
    fm.AddFile((file).c_str());
  }
  fm.Merge();
  
  RUNSVC_INFO("Merging to file: {} - done!",output_file);

  if(cleanUp){
    
  RUNSVC_INFO("Clean-up....");
    for(const auto& file : files_to_merge){
      svc::deleteFileIfExists(file);
    }
  }
}
