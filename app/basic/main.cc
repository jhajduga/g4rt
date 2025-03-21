/**
*
* \file main_setup_run_from_TOML.cc
* \author 
* \date 
* \brief 
*
*/

#include <cstdlib>
#include "Services.hh"
#include "cxxopts.h"
#include <pybind11/embed.h>
#include "toml.hh"
#include "colors.hh"
// // #include "LogSvc.hpp"
#include "WorldConstruction.hh"
#include "LogSvc.hpp"

int main(int argc, const char *argv[]) {


  pybind11::scoped_interpreter guard{};
  pybind11::module sys = pybind11::module::import("sys");
  sys.attr("path").attr("append")(std::string(PROJECT_PY_PATH));

  auto configSvc = Service<ConfigSvc>();  // initialize ConfigSvc for TOML parsing
  auto runSvc = Service<RunSvc>();        // get RunSvc for general App run configuration


    // Inicjalizacja loggera
    LogSvc::Init(argc, argv, "logs/app_main.log", loguru::Verbosity_MAX, 100);
    LogSvc::SetTerminalLogLevel(loguru::Verbosity_MAX);
    LogSvc::AddModuleLogFile("RunSvc", "logs/run_svc.log", loguru::Verbosity_MAX);
    LogSvc::AddModuleLogFile("MainModule", "logs/main_svc.log", loguru::Verbosity_MAX);

    LOGSVC_INFO("MainModule", "Program startuje.");
    LOGSVC_DEBUG("MainModule", "Debug log testowy.");
    LOGSVC_INFO("RunSvc", "Nowa wiadomość z runSVC");
    LOGSVC_INFO("MainModule", "Program kończy działanie.");


  if (argc > 1) {
  cxxopts::Options options(argv[0], "Text UI mode - command line options");
  try {
    options.positional_help("[optional args]");
    options.show_positional_help();

    options.add_options()("help", "Print help")("version", "Application version");

    options.add_options("Application run mode")
        ("g,BuildGeometry", "Only build geometry")
        ("f,FullSimulation", "Full simulation with geometry and specific physics")
        ("GeoExportCSV","Export geometry configuration to generic csv format")
        ("GeoExportGate","Export geometry configuration to GATE-like parameterization")
        ("GeoExportTFile","Export geometry to TFile")
        ("GeoExportGdml","Export geometry to GDML format")
        ("j,nCPU", "Number of threads", cxxopts::value<int>(), "NumCPU")
        ("n,nEvt", "Number of primary events (default value 1000)", cxxopts::value<int>(), "N")
        ("t,TOML", "Specify TOML job file", cxxopts::value<std::string>(), "FILE")
        ("o,OutputDir", "Specify output directory", cxxopts::value<std::string>(), "PATH")
        ("d,LogLevel", "Global level for logging", cxxopts::value<std::string>(), "LogLevel")
        ;

    auto results = options.parse(argc, argv);
    if (results.count("help")) {
      std::cout << options.help({"", "Application run mode"}) << std::endl;
      std::exit(EXIT_SUCCESS);
    }

    auto cmdopts = std::move(results);

    // GENERAL
    // --------------------------------------------------------------------
    if (cmdopts.count("version")) {
      std::cout << FGRN("Geant-RT verson v 1.0.0") << std::endl;
      std::exit(EXIT_SUCCESS);
      }

    if (cmdopts.count("d")) {
      auto logLevelStr = cmdopts["d"].as<std::string>();
      // LogSvc::DefaulLogLevel(logLevelStr);
    }

      // OPERATION
      // --------------------------------------------------------------------
      if (cmdopts.count("g")) 
        runSvc->AppMode(OperationalMode::BuildGeometry);

      if (cmdopts.count("f")) 
        runSvc->AppMode(OperationalMode::FullSimulation);

      if (cmdopts.count("GeoExportCSV"))  
        configSvc->SetValue("GeoSvc", "ExportGeometryToCsv", true);

      if (cmdopts.count("GeoExportTFile"))   
        configSvc->SetValue("GeoSvc", "ExportGeometryToTFile", true);

      if (cmdopts.count("GeoExportGate"))       
        configSvc->SetValue("GeoSvc", "ExportGeometryToGate", true);

      if (cmdopts.count("GeoExportGdml"))       
        configSvc->SetValue("GeoSvc", "ExportGeometryToGDML", true);

      if (cmdopts.count("j")) 
        runSvc->SetNofThreads(cmdopts["j"].as<int>());

      if (cmdopts.count("n")) {
        if (cmdopts["n"].as<int>() > std::numeric_limits<int>::max()) {
          std::cout << "[ERROR]:: Requested number of events " << cmdopts["n"].as<int>()
                    << " is greater than maximum allowed " << std::numeric_limits<int>::max() << std::endl;
          std::exit(EXIT_FAILURE);
        }
        configSvc->SetValue("RunSvc", "NumberOfEvents", static_cast<int>(cmdopts["n"].as<int>()),false);
      }

      if (cmdopts.count("t")){ 
        auto toml_file = cmdopts["t"].as<std::string>();
        if (svc::checkIfFileExist(toml_file)){
          std::cout << FGRN("[INFO]")<<":: TOML app run config file: "<< toml_file << std::endl;
          configSvc->ParseTomlFile(toml_file);
          configSvc->PrintTomlConfig();
        }
        else{
            std::string msg = "Path ( "+toml_file+") to TOML config file did not exist - please check your input!";
            svc::invalidArgumentError("main",msg);
        }
      }

      if (cmdopts.count("o")){ 
        configSvc->SetValue("RunSvc", "OutputDir", cmdopts["o"].as<std::string>());
      }

    } catch (const cxxopts::OptionException &e) {
      std::cout << "Error parsing options: " << e.what() << std::endl;
      std::exit(EXIT_FAILURE);
    } 
    auto world = WorldConstruction::GetInstance();
    runSvc->Initialize(world);
    runSvc->Run();
    runSvc->Finalize();

    loguru::shutdown();  // Zamknięcie loggera
  } else {
    G4cout << "[ERROR]:: Command line options missing (use '" << argv[0] << " --help' if needed)" << G4endl;
  }
  
  return EXIT_SUCCESS;
}
