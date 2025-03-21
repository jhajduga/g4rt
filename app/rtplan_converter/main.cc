/**
* Simple application for .dcm to .dat files conversion
*/

#include "Services.hh"
#include "cxxopts.h"
#include <pybind11/embed.h>
// #include "LogSvc.hpp"

// Helper functions to parse a pair from a string
std::pair<float, float> convertPair(const std::pair<std::string, std::string>& input) {
    try {
        float first = std::stof(input.first);
        float second = std::stof(input.second);
        return {first, second};
    } catch (const std::invalid_argument& e) {
        throw std::invalid_argument("Invalid argument: One or both strings cannot be converted to float.");
    } catch (const std::out_of_range& e) {
        throw std::out_of_range("Out of range: One or both strings represent a value out of float range.");
    }
}
std::pair<float, float> parsePair(const std::string& input) {
    std::istringstream stream(input);
    std::string first, second;
    if (std::getline(stream, first, ',') && std::getline(stream, second)) {
        return convertPair({first, second});
    }
    throw std::invalid_argument("Invalid pair format. Expected 'value1,value2'.");
}

int main(int argc, const char *argv[]) {

  pybind11::scoped_interpreter guard{};
  pybind11::module sys = pybind11::module::import("sys");
  sys.attr("path").attr("append")(std::string(PROJECT_PY_PATH));

  // SPDLOG_DEBUG("Initialize services");
  auto dicomSvc = Service<DicomSvc>();    //
  // SPDLOG_DEBUG("End of initialize services");

  // SPDLOG_INFO("Wellcome G4RT!");

  if (argc > 1) {
  cxxopts::Options options(argv[0], "Text UI mode - command line options");
  try {
    options.positional_help("[optional args]");
    options.show_positional_help();

    options.add_options()("help", "Print help")("version", "Application version");

    options.add_options("Application run mode")
        ("b,nBeams", "Number of beams (default value ALL)", cxxopts::value<int>(), "N")
        ("c,nCtrlPts", "Number of control points (default value ALL)", cxxopts::value<int>(), "N")
        ("nParticles", "Number of particles to be set in the plan (default value 1e3)", cxxopts::value<int>()->default_value("1000"), "N")
        ("f,File", "Specify RT-Plan file", cxxopts::value<std::string>(), "FILE")
        ("fieldCentre", "Perform Field centralization in AB sides", cxxopts::value<bool>()->default_value("false"))
        ("o,OutputDir", "Specify output directory", cxxopts::value<std::string>(), "PATH")
        ("fieldConstrain", "Input pair ([mm] in format: value1,value2)", cxxopts::value<std::string>());
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
      // SPDLOG_INFO("Geant-RT verson v 1.0.0");
      std::exit(EXIT_SUCCESS);
      }


      // USER OBLIGATORY PARAMETERS
      // --------------------------------------------------------------------
      auto rtplan_file = std::string();
      if (cmdopts.count("f")) 
        rtplan_file = cmdopts["f"].as<std::string>();
      if(rtplan_file.empty())
        svc::invalidArgumentError("main","Please specify RT-Plan file!");
      if(!svc::checkIfFileExist(rtplan_file))
        svc::invalidArgumentError("main","RT-Plan file not found!");
      auto output_dir = std::string();
      if (cmdopts.count("o")) 
        output_dir = cmdopts["o"].as<std::string>();
      if (output_dir.empty())
        svc::invalidArgumentError("main","Please specify output directory!");

      // USER OPTIONAL PARAMETERS
      // --------------------------------------------------------------------
      int usr_nBeams = -1000;
      if (cmdopts.count("b")) 
        usr_nBeams = cmdopts["b"].as<int>();
      int usr_nCtrlPts = -1000;
      if (cmdopts.count("c")) 
        usr_nCtrlPts = cmdopts["c"].as<int>();
      
      bool fieldConstrain = false;
      std::pair<float, float> field{-1,-1};
      if (cmdopts.count("fieldConstrain")){ 
        auto input = cmdopts["fieldConstrain"].as<std::string>();
        try {
            field = parsePair(input);
            std::cout << "Parsed pair: (" << field.first << ", " << field.second << ")\n";
            fieldConstrain = true;
        } catch (const std::invalid_argument& e) {
            std::cerr << e.what() << '\n';
            return 1;
        }
      }

      // OPERATION
      // --------------------------------------------------------------------
      auto fieldCentre = cmdopts["fieldCentre"].as<bool>();
      int nParticles = cmdopts["nParticles"].as<int>();
      auto centralize_ab = [&](std::vector<G4double>& mlc_a, std::vector<G4double>& mlc_b) {
        G4double min_a = 10000, min_b = 0;
        G4double max_a = 0, max_b = -10000;
        int min_leaf_y = -1, max_leaf_y = -1;
        for(size_t i_leaf=0; i_leaf < mlc_a.size(); i_leaf++){
          if(mlc_a.at(i_leaf) - mlc_b.at(i_leaf) != 0){ // check if mlc is not closed
            if(mlc_a.at(i_leaf) < min_a) min_a = mlc_a.at(i_leaf);
            if(mlc_b.at(i_leaf) < min_b) min_b = mlc_b.at(i_leaf);
            if(mlc_a.at(i_leaf) > max_a) max_a = mlc_a.at(i_leaf);
            if(mlc_b.at(i_leaf) > max_b) max_b = mlc_b.at(i_leaf);
          }
        }
        double shift_ab = 0;
        if (min_a<0 && max_b>0){
          shift_ab = -(max_b + min_a)/2;
        } else if (min_a<0 && max_b<=0) {
          shift_ab = -min_a/2;
        } else if (min_a>=0 && max_b>0) {
          shift_ab = -max_b/2;
        }
        std::cout << "Shift A-B: " << shift_ab << std::endl;
        for(size_t i_leaf=0; i_leaf < mlc_a.size(); i_leaf++){
          mlc_a.at(i_leaf) = mlc_a.at(i_leaf) + shift_ab;
          mlc_b.at(i_leaf) = mlc_b.at(i_leaf) + shift_ab;
        }
      };
      int not_x_cetral{0};
      auto centralize_x = [&](std::vector<G4double>& mlc_a, std::vector<G4double>& mlc_b) {
        int start_x_closed = 0, end_x_closed = 0;
        bool got_open = false;
        for(size_t i_leaf=0; i_leaf < mlc_a.size(); i_leaf++){
          if(mlc_a.at(i_leaf) - mlc_b.at(i_leaf) == 0 && !got_open){ // check if mlc is closed
            ++start_x_closed;
          } else {
            got_open=true;
          }
        }
        got_open = false;
        for(int i_leaf=mlc_a.size()-1; i_leaf >=0; i_leaf--){
          if(mlc_a.at(i_leaf) - mlc_b.at(i_leaf) == 0 && !got_open){ // check if mlc is closed
            ++end_x_closed;
          } else {
            got_open=true;
          }
        }
        if(start_x_closed != end_x_closed){
          std::cout << "NOT X CENTRAL" << start_x_closed << std::endl;
          ++not_x_cetral;
        }
        // std::cout << "Start #leafs closed: " << start_x_closed << std::endl;
        // std::cout << "End   #leafs closed: " << end_x_closed << std::endl;
      };
      
      auto isPassingFieldConstrain = [&](std::vector<G4double>& mlc_a,
                                        std::vector<G4double>& mlc_b) -> bool {
        if(field.first<0){
          std::cout << "[WARN]:: No field size specified!" << std::endl;
          return true;
        }
        G4double min_a = 10000, min_b = 0;
        G4double max_a = 0, max_b = -10000;
        double min_leaf_x = -1, max_leaf_x = -1;
        double min_leaf_y = -1, max_leaf_y = -1;
        double x_width = 3; // assume 3 mm width
        double current_x = 0;
        for(size_t i_leaf=0; i_leaf < mlc_a.size(); i_leaf++){
          current_x = i_leaf * x_width;
          if(mlc_a.at(i_leaf) - mlc_b.at(i_leaf) != 0){ // check if mlc is not closed
            if(min_leaf_x<0) min_leaf_x = current_x;
            if(max_leaf_x < current_x) max_leaf_x = current_x;
            if(mlc_a.at(i_leaf) < min_a) min_a = mlc_a.at(i_leaf);
            if(mlc_b.at(i_leaf) < min_b) min_b = mlc_b.at(i_leaf);
            if(mlc_a.at(i_leaf) > max_a) max_a = mlc_a.at(i_leaf);
            if(mlc_b.at(i_leaf) > max_b) max_b = mlc_b.at(i_leaf);
          }
        }
        min_leaf_y = min_a < min_b ? min_a : min_b;
        max_leaf_y = max_a > max_b ? max_a : max_b;
        std::cout << "X range: " << min_leaf_x << " : " << max_leaf_x << std::endl;
        std::cout << "Y range: " << min_leaf_y << " : " << max_leaf_y << std::endl;
        if (field.first > max_leaf_x - min_leaf_x &&
            field.second > max_leaf_y - min_leaf_y){
              std::cout << "Field Constrain PASSED" << std::endl;
            return true;
            }
            std::cout << "Field Constrain NOT PASSED: X:" << max_leaf_x - min_leaf_x << "  Y: " << max_leaf_y - min_leaf_y  << std::endl;
        return false;
      };

      auto write_dat_plan_file = [&]( std::string dat_plan_file,
                                      std::pair<double,double>& jaw_x,
                                      std::pair<double,double>& jaw_y, 
                                      std::vector<G4double>& mlc_a, 
                                      std::vector<G4double>& mlc_b) {
        if(mlc_a.empty() || mlc_b.empty()){
          // SPDLOG_ERROR("MLC data is empty!");
        }
        std::ofstream outFile(dat_plan_file);
        if (outFile.is_open()) {
          // Write data to file
          // NOTE: There are FIXED values!!!!
          outFile << "# Rotation: 0.0\n";
          outFile << "# Particles: "<< nParticles << "\n";
          outFile << "# Jaws: X1[mm],X2[mm],Y1[mm],Y2[mm]\n";
          outFile << jaw_x.first << "," << jaw_x.second << "," << jaw_y.first << "," << jaw_y.second << "\n";
          outFile << "# MLC: Y1[mm],Y2[mm]\n";
          for(size_t i_leaf=0; i_leaf < mlc_a.size(); i_leaf++)
            outFile << mlc_a.at(i_leaf) << "," << mlc_b.at(i_leaf) << "\n";

          // Close file
          outFile.close();
          // SPDLOG_INFO("Plan data written into file: {}", dat_plan_file);

      } else 
          // SPDLOG_ERROR("Unable to open file: {}", dat_plan_file);
          std::cout << "Unable to open file: " << dat_plan_file << std::endl;
      };


      dicomSvc->SetPlanFile(rtplan_file);
      auto nBeams = dicomSvc->GetRTPlanNumberOfBeams(rtplan_file);
      nBeams = (usr_nBeams > 0 && usr_nBeams < nBeams) ? usr_nBeams : nBeams;
      std::cout << "Number of beams: " << nBeams << std::endl;
      int passing_rate_counter{0};
      int cp_counter{0};
      for(int i_beam=0; i_beam<nBeams; i_beam++){
        auto nCtrlPts = dicomSvc->GetRTPlanNumberOfControlPoints(rtplan_file,i_beam);
        nCtrlPts = (usr_nCtrlPts > 0 && usr_nCtrlPts < nCtrlPts) ? usr_nCtrlPts : nCtrlPts;
        std::cout << "Beam " << i_beam << "  => Number of control points: " << nCtrlPts << std::endl;
        // NOTE: For all control points in the beam the jaws aperture
        // is defined in the first control point:
        auto jaw_x = dicomSvc->GetPlan()->ReadJawsAperture(rtplan_file,"X",i_beam,0);
        auto jaw_y = dicomSvc->GetPlan()->ReadJawsAperture(rtplan_file,"Y",i_beam,0);
        std::cout << "Jaws: " << jaw_x.first << "," << jaw_x.second << "," << jaw_y.first << "," << jaw_y.second << std::endl;
        for(int i_cp=0; i_cp < nCtrlPts; i_cp++){
          ++cp_counter;
          auto mlc_a = dicomSvc->GetPlan()->ReadMlcPositioning(rtplan_file,"Y1",i_beam,i_cp);
          std::cout << "MLC A: " << mlc_a.size() << std::endl;

          auto mlc_b = dicomSvc->GetPlan()->ReadMlcPositioning(rtplan_file,"Y2",i_beam,i_cp);
          std::cout << "MLC B: " << mlc_b.size() << std::endl;

          std::string dat_plan_file = svc::getFileName(rtplan_file);
          dat_plan_file = output_dir + "/"+dat_plan_file+"_beam"+std::to_string(i_beam)+"_cp"+std::to_string(i_cp)+".dat";
          if (fieldCentre){
            centralize_ab(mlc_a, mlc_b);
            centralize_x(mlc_a, mlc_b);
          }
          auto passed = true;
          if (fieldConstrain)
            passed = isPassingFieldConstrain(mlc_a, mlc_b);
          if (passed){
            write_dat_plan_file(dat_plan_file,jaw_x,jaw_y,mlc_a,mlc_b);
            ++passing_rate_counter;
          }
          mlc_a.clear();
          mlc_b.clear();
          
        }
      }
      std::cout << "#Processed CP: " << cp_counter << " filtered and saved: " << passing_rate_counter << "("<< double(passing_rate_counter)*100/cp_counter <<"%)" << std::endl;
      std::cout << "# Not centralized in X: " << not_x_cetral << std::endl;
    } catch (const cxxopts::OptionException &e) {
      std::cout << "Error parsing options: " << e.what() << std::endl;
      std::exit(EXIT_FAILURE);
    } 
  } else {
    G4cout << "[ERROR]:: Command line options missing (use '" << argv[0] << " --help' if needed)" << G4endl;
  }
  
  return EXIT_SUCCESS;
}
