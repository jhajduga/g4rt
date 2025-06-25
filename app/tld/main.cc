
#include <cstdlib>
#include "Services.hh"
#include "cxxopts.h"
#include <pybind11/embed.h>
#include "toml.hh"
#include "LogSvc.hh"
#include "TLDWorldConstruction.hh"

int main(int argc, const char *argv[]) {
  // Force POSIX "C" locale to ensure consistent scientific notation (e.g., 1.23e-12).
  // In some locales (e.g., pl_PL.UTF-8), numerical formatting functions may emit
  // invalid or locale-specific formats that ROOT or GDML parsers can't read.
  // This fixes cases where exponent notation is lost or misformatted.
  setenv("LC_ALL", "C", 1);


  pybind11::scoped_interpreter guard{};
  pybind11::module sys = pybind11::module::import("sys");
  sys.attr("path").attr("append")(std::string(PROJECT_PY_PATH));
  
  SPDLOG_DEBUG("Initialize services");
  auto configSvc = Service<ConfigSvc>();  // initialize ConfigSvc for TOML parsing
  auto runSvc = Service<RunSvc>();        // get RunSvc for general App run configuration

  SPDLOG_INFO("Wellcome G4RT!");

  if (argc > 1) {
  cxxopts::Options options(argv[0], "Text UI mode - command line options");
  try {
    options.positional_help("[optional args]");
    options.show_positional_help();

    options.add_options()("help", "Print help")("version", "Application version");

    options.add_options("Application run mode")
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
    if (cmdopts.count("d")) {
      auto logLevelStr = cmdopts["d"].as<std::string>();
      LogSvc::DefaulLogLevel(logLevelStr);
    }

      // OPERATION
      // --------------------------------------------------------------------
      runSvc->AppMode(OperationalMode::FullSimulation);

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
          std::cout <<"[INFO]:: TOML app run config file: "<< toml_file << std::endl;
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
    runSvc->Initialize(TLDWorldConstruction::GetInstance());
    runSvc->Run();
    runSvc->Finalize();
  } else {
    G4cout << "[ERROR]:: Command line options missing (use '" << argv[0] << " --help' if needed)" << G4endl;
  }
  
  return EXIT_SUCCESS;
}


