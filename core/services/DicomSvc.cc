#include "DicomSvc.hh"
#include "Services.hh"
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>
// #include "LogSvc.hpp"

namespace py = pybind11;
using namespace py::literals;

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Initializes the DICOM service based on the plan file type.
 *
 * This function selects and creates the appropriate plan configuration for processing a radiation therapy plan file.
 * A custom plan is instantiated for ".dat" files, while a standard DICOM plan is used for ".dcm" files.
 * If an unsupported file extension is provided, a fatal error is raised.
 *
 * @param planFileType The plan file's extension (e.g., "dat" or "dcm").
 * @throws G4Exception if the file extension is not recognized.
 */
void DicomSvc::Initialize(const std::string& planFileType){
  // LOGSVC_INFO("DicomSvc initalization...");
  if(planFileType == "dat")
    m_plan = std::make_unique<ICustomPlan>();
  else if(planFileType == "dcm")
    m_plan = std::make_unique<IDicomPlan>();
  else {
    G4String msg = "Unknown plan file extension: " + planFileType + "Supported: .dat, .dcm";
    // LOGSVC_CRITICAL(msg.data());
    G4Exception("DicomSvc", "Initialize", FatalErrorInArgument, msg);
  }

  // TODO refacto the following as the context specific code...
  //
  // auto rtplanMlcReader = py::module::import("dicom_rtplan_mlc");
  // auto beams_counter = rtplanMlcReader.attr("return_number_of_beams")(m_rtplan_file).cast<int>();
  // // LOGSVC_INFO("Found #{} beams",beams_counter);
  // for(int i=0; i<beams_counter;++i){
  //   unsigned nCP = rtplanMlcReader.attr("return_number_of_controlpoints")(m_rtplan_file,i).cast<unsigned>();
  //   m_rtplan_beam_n_control_points.emplace_back(nCP);
  //   // LOGSVC_INFO("Found #{} control poinst for {} beam",nCP,i);
  // }
}

void DicomSvc::SetPlanFile(const std::string& plan_file){
  m_rtplan_file = plan_file;
  Initialize(svc::getFileExtenstion(m_rtplan_file));
}

////////////////////////////////////////////////////////////////////////////////
///
unsigned DicomSvc::GetTotalNumberOfControlPoints() const {
  return std::accumulate(m_rtplan_beam_n_control_points.begin(), m_rtplan_beam_n_control_points.end(), 0);
}

////////////////////////////////////////////////////////////////////////////////
///
DicomSvc *DicomSvc::GetInstance() {
  static DicomSvc instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Retrieves the position of a specified jaw from a DICOM plan.
 *
 * This function determines the position of a jaw (either "X1", "X2", "Y1", or "Y2") for a given beam and control point
 * in the provided DICOM plan file. It validates the jaw name input and, if valid, employs a corresponding Python module to retrieve
 * the position. An invalid jaw name triggers a fatal error via G4Exception.
 *
 * @param planFile The file path to the DICOM plan.
 * @param jawName The name of the jaw ("X1", "X2", "Y1", or "Y2").
 * @param beamIdx The index of the beam in the plan.
 * @param controlpointIdx The index of the control point in the specified beam.
 * @return double The retrieved jaw position value.
 */
double IDicomPlan::ReadJawPossition(const std::string& planFile, const std::string& jawName, int beamIdx, int controlpointIdx) const{
  //// LOGSVC_INFO("Reading the Jaws configuration from {}",planFile);
  //// LOGSVC_INFO("JawName: {}, beamIdx: {}, controlpointIdx: {}",jawName,beamIdx,controlpointIdx);
  if(jawName!="X1" && jawName!="X2" && jawName!="Y1" && jawName!="Y2")
    G4Exception("IDicomPlan", "GetJawPossition", FatalErrorInArgument, "Wrong jaw name input given!");

  auto rtplanJawsReader = py::module::import("dicom_rtplan_jaws");
  // auto beams_counter = rtplanJawsReader.attr("return_number_of_beams")(planFile);
  // const int number_of_beams = beams_counter.cast<int>();
  // auto controlpoints_counter = rtplanJawsReader.attr("return_number_of_controlpoints")(planFile, number_of_beams);
  // const int number_of_controlpoints = controlpoints_counter.cast<int>();
  // auto jaws_counter = rtplanJawsReader.attr("return_number_of_jaws")(planFile);
  // const int number_of_jaws = jaws_counter.cast<int>();
  // std::cout << "We have " << number_of_beams << " beams, " << number_of_controlpoints << " checkpoints and "
  //           << number_of_jaws << " jaws." << std::endl;
  int jawIdx = 0; // X1
  if (jawName == "X2") jawIdx = 1;
  else if (jawName == "Y1") jawIdx = 2;
  else if (jawName == "Y2") jawIdx = 3;
  return rtplanJawsReader.attr("return_position")(planFile, beamIdx, controlpointIdx, jawIdx).cast<double>();
}

////////////////////////////////////////////////////////////////////////////////
///
std::pair<double,double> IDicomPlan::ReadJawsAperture(const std::string& planFile,const std::string& side,int beamIdx, int controlpointIdx){
  if(side=="X"){
    auto x1 = ReadJawPossition(planFile,"X1",beamIdx,controlpointIdx);
    auto x2 = ReadJawPossition(planFile,"X2",beamIdx,controlpointIdx);
    return std::make_pair(x1,x2);
  }else if(side=="Y"){
    auto y1 = ReadJawPossition(planFile,"Y1",beamIdx,controlpointIdx);
    auto y2 = ReadJawPossition(planFile,"Y2",beamIdx,controlpointIdx);
    return std::make_pair(y1,y2);
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void IPlan::AcknowledgeJawsAperture(const std::string& side, const std::pair<double,double>& jawsAperture) const {
  // TODO
  // Check the Jaws type and verify if complete and correct information is read-in;
  // if not, throw an exception
}


////////////////////////////////////////////////////////////////////////////////
///
void IPlan::AcknowledgeMlcPositioning(const std::string& side, const std::vector<G4double>& mlc_positioning) const {
  // TODO
  // Check the MLC type and verify if complete and correct information is read-in;
  // if not, throw an exception
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Retrieves the MLC positioning configuration for the specified DICOM RT plan.
 *
 * This function extracts the multi-leaf collimator (MLC) positioning data for a given plan file.
 * It verifies that the provided side is either "Y1" or "Y2", then uses a Python module to determine
 * the number of leaves and obtain their positions for the specified beam and control point. The
 * retrieved positions are acknowledged before being returned.
 *
 * @param planFile Path to the DICOM RT plan file.
 * @param side The MLC side from which to retrieve positions; valid values are "Y1" and "Y2".
 *             An invalid value triggers a fatal error.
 * @param beamIdx Index of the beam in the plan.
 * @param controlpointIdx Index of the control point corresponding to the beam.
 * @return std::vector<G4double> A vector containing the extracted leaf positions.
 *
 * @exception G4Exception Thrown with FatalErrorInArgument if the side parameter is not "Y1" or "Y2".
 */
std::vector<G4double> IDicomPlan::ReadMlcPositioning(const std::string& planFile, const std::string& side, int beamIdx, int controlpointIdx){
  // // LOGSVC_INFO("Reading the MLC configuration from {}",planFile);
  // // LOGSVC_INFO("Side: {}, beamIdx: {}, controlpointIdx: {}",side,beamIdx,controlpointIdx);
  if(side!="Y1" && side!="Y2")
    G4Exception("IDicomPlan", "GetMlcPositioning", FatalErrorInArgument, "Wrong input side given!");
  std::vector<G4double> mlcPositioning;
  auto rtplanMlcReader = py::module::import("dicom_rtplan_mlc");
  // auto beams_counter = rtplanMlcReader.attr("return_number_of_beams")(planFile);
  // const int number_of_beams = beams_counter.cast<int>();
  // auto controlpoints_counter = rtplanMlcReader.attr("return_number_of_controlpoints")(planFile, number_of_beams);
  // const int number_of_controlpoints = controlpoints_counter.cast<int>();
  auto leaves_counter = rtplanMlcReader.attr("return_number_of_leaves")(planFile);
  const int number_of_leaves = leaves_counter.cast<int>();
  // std::cout << "We have " << number_of_beams << " beams, " << number_of_controlpoints << " checkpoints and "
  //           << number_of_leaves << " leaves." << std::endl;
  auto leavesPositions = rtplanMlcReader.attr("return_possition")(planFile, side, beamIdx, controlpointIdx, number_of_leaves).cast<py::array_t<double>>();
  py::buffer_info acceser = leavesPositions.request();
  double *accesableLeavesPositions = (double *) acceser.ptr;
  for (int i = 0; i < acceser.size; i++) {
    mlcPositioning.emplace_back(accesableLeavesPositions[i]);
  }
  AcknowledgeMlcPositioning(side, mlcPositioning);
  return std::move(mlcPositioning);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Reads and extracts a specified jaw position from a custom plan file.
 *
 * This function opens the given plan file and searches for the line containing the "Jaws" header.
 * After locating the header, it reads the next line expecting four comma-separated values corresponding
 * to the jaw positions (X1, X2, Y1, Y2). It returns the double value of the jaw specified by @p jawName.
 * If the file cannot be opened, the header is missing, or the line cannot be parsed, the function raises
 * a fatal error via G4Exception.
 *
 * The parameters @p beamIdx and @p controlpointIdx are included for interface consistency and are not currently used.
 *
 * @param planFile Path to the custom plan file.
 * @param jawName The jaw to retrieve ("X1", "X2", "Y1", or "Y2").
 * @param beamIdx Index of the beam (unused).
 * @param controlpointIdx Index of the control point (unused).
 * @return double The value of the specified jaw position, or 0.0 if @p jawName is unrecognized.
 */
double ICustomPlan::ReadJawPossition(const std::string& planFile, const std::string& jawName, int beamIdx, int controlpointIdx) const{
  std::ifstream file(planFile);
  if (!file.is_open()) {
    G4String msg = "Could not open file: " + planFile;
    // LOGSVC_CRITICAL(msg.data());
    G4Exception("ICustomPlan", "GetMlcPositioning", FatalErrorInArgument, msg);
  }
  auto findJawHeader = [&file]() -> int {
    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        ++lineNumber;
        if (line.find("Jaws") != std::string::npos) {
            return lineNumber;
        }
    }
    return -1; // Return -1 if "Jaws" not found
  };

  auto jaw_header = findJawHeader();
  if (jaw_header<0){
    G4String msg = "Could find Jaws header in file: " + planFile;
    // LOGSVC_CRITICAL(msg.data());
    G4Exception("ICustomPlan", "ReadJawPossition", FatalErrorInArgument, msg); 
  }
  std::string line;
  std::string x1,x2,y1,y2;
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    // Get the values as strings separated by a comma
    if (std::getline(iss, x1, ',') 
        && std::getline(iss, x2, ',') 
        && std::getline(iss, y1, ',')
        && std::getline(iss, y2, ',') ) {
      break;
    } else {
      G4String msg = "Could not parse line: " + line;
      // LOGSVC_CRITICAL(msg.data());
      G4Exception("ICustomPlan", "ReadJawPossition", FatalErrorInArgument, msg);
    }
  }
  file.close();
  if(jawName=="X1"){
    return std::stod(x1);
  } else if(jawName=="X2"){
    return std::stod(x2);
  } else if(jawName=="Y1"){
    return std::stod(y1);
  } else if(jawName=="Y2"){
    return std::stod(y2);
  }
  return 0.;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Retrieves the jaw aperture positions from a custom plan file.
 *
 * Reads the pair of jaw positions (either X1/X2 for the "X" side or Y1/Y2 for the "Y" side) for the specified beam and control point
 * from the provided plan file. If an invalid side is specified, the function terminates the application.
 *
 * @param planFile Path to the custom plan file.
 * @param side The jaw side to read ("X" or "Y").
 * @param beamIdx Index of the beam in the plan.
 * @param controlpointIdx Index of the control point within the beam.
 * @return std::pair<double,double> A pair containing the two jaw positions for the specified side.
 */
std::pair<double,double> ICustomPlan::ReadJawsAperture(const std::string& planFile,const std::string& side,int beamIdx, int controlpointIdx){
  std::pair<double,double> jawsSideAperture;
  if(side=="X"){
    auto x1 = ReadJawPossition(planFile,"X1",beamIdx,controlpointIdx);
    auto x2 = ReadJawPossition(planFile,"X2",beamIdx,controlpointIdx);
    // LOGSVC_INFO("JAWS:: X1={} mm, X2={} mm",x1,x2);
    jawsSideAperture = std::make_pair(x1,x2);
  } else if(side=="Y"){
    auto y1 = ReadJawPossition(planFile,"Y1",beamIdx,controlpointIdx);
    auto y2 = ReadJawPossition(planFile,"Y2",beamIdx,controlpointIdx);
    // LOGSVC_INFO("JAWS:: Y1={} mm, Y2={} mm",y1,y2);
    jawsSideAperture = std::make_pair(y1,y2);
  } else {
      // LOGSVC_ERROR("ICustomPlan::ReadJawsAperture: Unknown side: {}", side);
      std::exit(EXIT_FAILURE);
  }
  AcknowledgeJawsAperture(side,jawsSideAperture);
  return std::move(jawsSideAperture);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Reads MLC positioning data from a custom plan file.
 *
 * This function opens the specified plan file and searches for the "MLC" header to locate
 * comma-separated positioning values for both the Y1 and Y2 sides. It validates that the
 * provided side is either "Y1" or "Y2" and throws a fatal error via G4Exception if this is
 * not the case, if the file cannot be opened, or if the "MLC" header is not found.
 *
 * After locating the header, the function reads the remaining lines, parses two comma-separated
 * values per line (one for each side), and converts them to doubles. It then acknowledges the
 * positioning configuration for the specified side and returns the corresponding vector of values.
 *
 * Note: The beamIdx and controlpointIdx parameters are part of the interface but are currently unused.
 *
 * @param planFile The path to the custom plan file containing the MLC configuration.
 * @param side The side to extract the MLC positioning from ("Y1" or "Y2").
 * @param beamIdx Unused parameter for the beam index.
 * @param controlpointIdx Unused parameter for the control point index.
 * @return std::vector<G4double> A vector of MLC positioning values corresponding to the specified side.
 * @throws G4Exception if the side is invalid, the file cannot be opened, or the "MLC" header is absent.
 */
std::vector<G4double> ICustomPlan::ReadMlcPositioning(const std::string& planFile, const std::string& side, int beamIdx, int controlpointIdx){
  // LOGSVC_INFO("Reading the MLC {} side configuration from {}",side,planFile);
  if(side!="Y1" && side!="Y2"){
    G4String msg = "Wrong input side given (" + side + ")! Allowed values are 'Y1' and 'Y2'";
    // LOGSVC_CRITICAL(msg.data());
    G4Exception("ICustomPlan", "GetMlcPositioning", FatalErrorInArgument, msg);
  }
  std::ifstream file(planFile);
  if (!file.is_open()) {
    G4String msg = "Could not open file: " + planFile;
    // LOGSVC_CRITICAL(msg.data());
    G4Exception("ICustomPlan", "GetMlcPositioning", FatalErrorInArgument, msg);
  }

  auto findMLCheader = [&file]() -> int {
    std::string line;
    int lineNumber = 0;
    while (std::getline(file, line)) {
        ++lineNumber;
        if (line.find("MLC") != std::string::npos) {
            return lineNumber;
        }
    }
    return -1; // Return -1 if "MLC" not found
  };

  std::string line;
  std::vector<double> mlc_y1, mlc_y2;
  auto mlc_header = findMLCheader();
  if (mlc_header<0){
    G4String msg = "Could find MLC header in file: " + planFile;
    // LOGSVC_CRITICAL(msg.data());
    G4Exception("ICustomPlan", "GetMlcPositioning", FatalErrorInArgument, msg); 
  }
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string value_y1, value_y2;
    // Get the values as strings separated by a comma
    if (std::getline(iss, value_y1, ',') && std::getline(iss, value_y2)) {
      // Convert string to double and add to the respective vectors
      mlc_y1.push_back(std::stod(value_y1));
      mlc_y2.push_back(std::stod(value_y2));
    }
  }
  file.close();
  AcknowledgeMlcPositioning(side,side=="Y1" ? mlc_y1 : mlc_y2);
  return side=="Y1" ? std::move(mlc_y1) : std::move(mlc_y2);
}

////////////////////////////////////////////////////////////////////////////////
///
double DicomSvc::GetRTPlanAngle(int current_beam, int current_controlpoint) const {
  auto rtplanAngleReader = py::module::import("dicom_rtplan_angle");
  // Zakomentowane części kodu - na przyszłość przy większej ilości runów
  // py::function beams_counter = rtplanAngleReader.attr("return_number_of_beams")(m_rtplan_file);
  // int number_of_beams = beams_counter.cast<int>();
  // py::function controlpoints_counter = rtplanAngleReader.attr("return_number_of_controlpoints")(m_rtplan_file, number_of_beams);
  // int number_of_controlpoints = controlpoints_counter.cast<int>();
  auto angleInControlpoint = rtplanAngleReader.attr("return_angle_in_controlpoint")(m_rtplan_file, current_beam,
                                                                              current_controlpoint).cast<double>();
  std::cout <<angleInControlpoint<< '\n';
  return angleInControlpoint;
}
////////////////////////////////////////////////////////////////////////////////
///
double DicomSvc::GetRTPlanDose(int current_beam, int current_controlpoint) const {

  auto rtplanDoseReader = py::module::import("dicom_rtplan_dose");
  // Zakomentowane części kodu - na przyszłość przy większej ilości runów
  // py::function beams_counter = rtplanDoseReader.attr("return_number_of_beams")(m_rtplan_file);
  // int number_of_beams = beams_counter.cast<int>();
  // py::function controlpoints_counter = rtplanDoseReader.attr("return_number_of_controlpoints")(m_rtplan_file, number_of_beams);
  // int number_of_controlpoints = controlpoints_counter.cast<int>();
  auto doseInControlpoint = rtplanDoseReader.attr("return_dose_during_specified_controlpoint")(m_rtplan_file, current_beam,
                                                                                  current_controlpoint).cast<double>();
  std::cout <<doseInControlpoint<< '\n';
  return doseInControlpoint;
}

////////////////////////////////////////////////////////////////////////////////
///
unsigned DicomSvc::GetRTPlanNumberOfBeams(const std::string& planFile) const {
  auto rtplanMlcReader = py::module::import("dicom_rtplan_mlc");
  auto beams_counter = rtplanMlcReader.attr("return_number_of_beams")(planFile);
  return beams_counter.cast<unsigned>();
}

////////////////////////////////////////////////////////////////////////////////
///
unsigned DicomSvc::GetRTPlanNumberOfControlPoints(const std::string& planFile,unsigned beamNumber) const{
  auto rtplanMlcReader = py::module::import("dicom_rtplan_mlc");
  return rtplanMlcReader.attr("return_number_of_controlpoints")(planFile,beamNumber).cast<unsigned>();
}

////////////////////////////////////////////////////////////////////////////////
///
void DicomSvc::ExportPatientToCT(const std::string& series_csv_path, const std::string& output_path) const {

    // PyGILState_Release(gstate);
    m_ct_svc.set_paths(output_path);
    m_ct_svc.create_ct_series(series_csv_path);
    m_ct_svc.~ICtSvc();
  }

////////////////////////////////////////////////////////////////////////////////
///
ControlPointConfig DicomSvc::GetControlPointConfig(int id, const std::string& planFile){
  auto dicomSvc = DicomSvc::GetInstance();
  if(!dicomSvc->Initialized()){
    dicomSvc->Initialize(svc::getFileExtenstion(planFile));
  }
  auto file = planFile;
  if(file.front() != '/'){ // the path is not absolute
    std::string data_path = PROJECT_DATA_PATH;
    file = data_path + "/" + file;
  }
  return dicomSvc->m_plan->GetControlPointConfig(id, file);
}

////////////////////////////////////////////////////////////////////////////////
///
ControlPointConfig IDicomPlan::GetControlPointConfig(int id, const std::string& planFile){
  // TODO
  // extract from planFile, below is dummy mockup
  auto config = ControlPointConfig(id, 1000, 0.);
  config.PlanFile = planFile;
  config.FieldType = "RTPlan"; // TODO FieldType::RTPlan;
  config.FieldSizeA = 23.0; // Temp
  config.FieldSizeB = 35.0; // Temp 
  return std::move(config);
}

////////////////////////////////////////////////////////////////////////////////
///
ControlPointConfig ICustomPlan::GetControlPointConfig(int id, const std::string& planFile){
  auto nEvents = GetNEvents(planFile);
  auto rotation = GetRotation(planFile);
  auto config = ControlPointConfig(id, nEvents, rotation);
  config.PlanFile = planFile;
  config.FieldType = "CustomPlan"; // TODO FieldType::CustomPlan;
  config.FieldSizeA = 23.0; // Temp
  config.FieldSizeB = 35.0; // Temp 
  return std::move(config);
}
////////////////////////////////////////////////////////////////////////////////
///
int ICustomPlan::GetNEvents(const std::string& planFile) {
  std::string svalue;
  std::string line;
  std::ifstream file(planFile.c_str());
  if (file.is_open()) {
    while (getline(file, line)){
      if (line.length() > 0 && (line.rfind("# Particles:",0) == 0)) {
        std::istringstream ss(line);
        while (getline(ss, svalue,':')){
          if(svalue.rfind("#",0)!=0){
            return static_cast<int>(std::stod(svalue.c_str())); 
            }
          }
        } 
      }
    }
  return 0; 
}

////////////////////////////////////////////////////////////////////////////////
///
double ICustomPlan::GetRotation(const std::string& planFile) {
  std::string svalue;
  std::string line;
  std::ifstream file(planFile.c_str());
  if (file.is_open()) {
    while (getline(file, line)){
      if (line.length() > 0 && (line.rfind("# Rotation:",0) == 0)) {
        std::istringstream ss(line);
        while (getline(ss, svalue,':')){
          if(svalue.rfind("#",0)!=0){
            return std::stod(svalue.c_str()); 
            }
          }
        } 
      }
    }
  return 0.; 
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Retrieves the Hounsfield scale value for a specified material.
 *
 * The function loads a configuration from a TOML file (if it hasn't been loaded already) and retrieves the Hounsfield
 * scale value for the given material from the "Hounsfield" section. If the value is found and normalization is requested,
 * the raw value is linearly scaled to a range of [0.02, 0.98] using the vacuum and tungsten reference values. If the
 * material is not present in the configuration, a default value of 0 is returned.
 *
 * @param materialName Name of the material to query.
 * @param normalized   If true, returns the normalized Hounsfield scale value.
 * @return The material's Hounsfield scale value, normalized if requested.
 */
double DicomSvc::GetHounsfieldScaleValue(const std::string& materialName, bool normalized){
  static std::unique_ptr<toml::table> tconfig;
  if(!tconfig){
    auto hausfieldMaterialMapFile = std::string(PROJECT_DATA_PATH) + "/config/hounsfield_scale_120keV.toml";
    tconfig = std::make_unique<toml::table>(toml::parse_file(hausfieldMaterialMapFile));
    // LOGSVC_INFO("Reading from Hounsfield Material Map File: {}",hausfieldMaterialMapFile);
  }
  const toml::table& config = *tconfig;
  G4double hu_value = 0.;
  // Access values under [Hounsfield] section
  if (config["Hounsfield"].as_table()->find(materialName)!= config["Hounsfield"].as_table()->end()){
    hu_value = config["Hounsfield"][materialName].value_or(0);
    if(normalized){ // normalized (min=0.02, max=0.98 scaling)
      G4double hu_min = config["Hounsfield"]["Vacuum"].value_or(-1000);
      G4double hu_max = config["Hounsfield"]["Tungsten"].value_or(3072);
      G4double new_min = 0.02;
      G4double new_max = 0.98;
      return (hu_value-hu_min)/(hu_max-hu_min) * (new_max-new_min) + new_min;
    }
  }
  return hu_value;
}


