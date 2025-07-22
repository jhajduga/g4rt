#include "DicomSvc.hh"
#include "Services.hh"
#include <pybind11/embed.h>
#include <pybind11/stl.h>
#include <pybind11/numpy.h>

namespace py = pybind11;
using namespace py::literals;

class ICtSvc::Impl {
  public:
      /**
       * @brief Constructs the implementation by instantiating the Python dicom_ct.CtSvc object.
       *
       * Imports the Python module `dicom_ct` and creates an instance of its `CtSvc` class, storing the resulting Python object for later use.
       */
      Impl()
          : m_py_dicom_ct(py::reinterpret_borrow<py::object>(
                py::module::import("dicom_ct").attr("CtSvc")().ptr()))
      {}
  
      /**
       * @brief Releases the Python object associated with the CT service upon destruction.
       */
      ~Impl() {
          m_py_dicom_ct.release();
      }
  
      /**
       * @brief Sets the output and project paths for the Python CT service.
       *
       * Configures the underlying Python `CtSvc` object with the specified output path and a predefined project data path.
       *
       * @param output_path The directory path where output data will be stored.
       */
      void set_paths(const std::string& output_path) const {
          m_py_dicom_ct.attr("set_output_path")(output_path);
          m_py_dicom_ct.attr("set_project_path")(PROJECT_DATA_PATH);
      }
  
      /**
       * @brief Creates a CT series from a CSV file using the underlying Python service.
       *
       * @param series_csv_path Path to the CSV file containing CT series data.
       */
      void create_ct_series(const std::string& series_csv_path) const {
          m_py_dicom_ct.attr("create_ct_series")(series_csv_path);
      }
  
  private:
      py::object m_py_dicom_ct;
  };
  

  /**
 * @brief Constructs an ICtSvc instance and initializes the underlying Python CT service.
 *
 * Instantiates the internal implementation, which loads and manages the Python `dicom_ct.CtSvc` object for CT series operations.
 */
ICtSvc::ICtSvc()
    : m_impl(std::make_unique<Impl>())
{}

/**
 * @brief Destroys the ICtSvc instance and releases associated resources.
 */
ICtSvc::~ICtSvc() = default;

/**
 * @brief Sets the output and project paths for CT series processing.
 *
 * Configures the underlying Python CT service with the specified output path.
 *
 * @param output_path Path to the directory where output files will be stored.
 */
void ICtSvc::set_paths(const std::string& output_path) const {
    m_impl->set_paths(output_path);
}

/**
 * @brief Creates a CT series from a CSV file using the underlying Python service.
 *
 * @param series_csv_path Path to the CSV file containing CT series data.
 */
void ICtSvc::create_ct_series(const std::string& series_csv_path) const {
    m_impl->create_ct_series(series_csv_path);
}


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Initializes the plan handler based on the specified plan file type.
 *
 * Selects and creates the appropriate plan handler (`ICustomPlan` for `.dat` files or `IDicomPlan` for `.dcm` files) according to the provided file type. Throws a fatal exception if the file type is unsupported.
 *
 * @param planFileType The file extension indicating the plan type ("dat" or "dcm").
 */
void DicomSvc::Initialize(const std::string& planFileType){
  
                LOGSVC_INFO("DcmSvc","DicomSvc initalization...");
  if(planFileType == "dat")
    m_plan = std::make_unique<ICustomPlan>();
  else if(planFileType == "dcm")
    m_plan = std::make_unique<IDicomPlan>();
  else {
    G4String msg = "Unknown plan file extension: " + planFileType + "Supported: .dat, .dcm";
    LOGSVC_FATAL("DcmSvc",msg.data());
    G4Exception("DicomSvc", "Initialize", FatalErrorInArgument, msg);
  }

  // TODO refacto the following as the context specific code...
  //
  // auto rtplanMlcReader = py::module::import("dicom_rtplan_mlc");
  // auto beams_counter = rtplanMlcReader.attr("return_number_of_beams")(m_rtplan_file).cast<int>();
  // 
  //              LOGSVC_INFO("DcmSvc","Found #{} beams",beams_counter);
  // for(int i=0; i<beams_counter;++i){
  //   unsigned nCP = rtplanMlcReader.attr("return_number_of_controlpoints")(m_rtplan_file,i).cast<unsigned>();
  //   m_rtplan_beam_n_control_points.emplace_back(nCP);
  //   
  //              LOGSVC_INFO("DcmSvc","Found #{} control poinst for {} beam",nCP,i);
  // }
}

/**
 * @brief Sets the radiotherapy plan file and initializes the appropriate plan handler.
 *
 * Updates the current plan file and initializes the plan object based on the file's extension.
 *
 * @param plan_file Path to the RT plan file (.dcm or .dat).
 */
void DicomSvc::SetPlanFile(const std::string& plan_file){
  m_rtplan_file = plan_file;
  Initialize(svc::getFileExtenstion(m_rtplan_file));
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the total number of control points across all beams in the current RT plan.
 *
 * @return Total count of control points summed over all beams.
 */
unsigned DicomSvc::GetTotalNumberOfControlPoints() const {
  return std::accumulate(m_rtplan_beam_n_control_points.begin(), m_rtplan_beam_n_control_points.end(), 0);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the singleton instance of the DicomSvc service.
 *
 * Ensures that only one instance of DicomSvc exists throughout the application.
 * @return Pointer to the singleton DicomSvc instance.
 */
DicomSvc *DicomSvc::GetInstance() {
  static DicomSvc instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Retrieves the position of a specified jaw from a DICOM RT plan file.
 *
 * Queries the Python module `dicom_rtplan_jaws` to obtain the position of the given jaw ("X1", "X2", "Y1", or "Y2") for a specific beam and control point in the provided plan file.
 *
 * @param jawName Name of the jaw ("X1", "X2", "Y1", or "Y2").
 * @param beamIdx Index of the beam.
 * @param controlpointIdx Index of the control point.
 * @return double Position of the specified jaw.
 *
 * @throws G4Exception If an invalid jaw name is provided.
 */
double IDicomPlan::ReadJawPossition(const std::string& planFile, const std::string& jawName, int beamIdx, int controlpointIdx) const{
  LOGSVC_INFO("DcmSvc","Reading the Jaws configuration from {}",planFile);
  LOGSVC_INFO("DcmSvc","JawName: {}, beamIdx: {}, controlpointIdx: {}",jawName,beamIdx,controlpointIdx);
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
/**
 * @brief Reads the aperture positions for a specified jaw side from a DICOM RT plan file.
 *
 * Retrieves the positions of both jaws on the given side ("X" or "Y") for the specified beam and control point.
 *
 * @param side Jaw side to read ("X" for X1/X2, "Y" for Y1/Y2).
 * @return Pair of jaw positions for the specified side.
 */
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
/**
 * @brief Placeholder for validating jaw aperture data.
 *
 * Intended to verify the completeness and correctness of jaw aperture information for the specified side.
 * Should throw an exception if the data is invalid or incomplete.
 */
void IPlan::AcknowledgeJawsAperture(const std::string& side, const std::pair<double,double>& jawsAperture) const {
  // TODO
  // Check the Jaws type and verify if complete and correct information is read-in;
  // if not, throw an exception
}


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Placeholder for validating MLC positioning data.
 *
 * Intended to check the MLC type and verify that the provided positioning information is complete and correct. Should throw an exception if validation fails.
 */
void IPlan::AcknowledgeMlcPositioning(const std::string& side, const std::vector<G4double>& mlc_positioning) const {
  // TODO
  // Check the MLC type and verify if complete and correct information is read-in;
  // if not, throw an exception
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Reads the MLC (multi-leaf collimator) leaf positions for a specified side, beam, and control point from a DICOM RT plan file.
 *
 * Imports the Python module `dicom_rtplan_mlc` to retrieve the number of leaves and their positions for the given parameters. Validates the side input and returns the MLC positions as a vector of doubles.
 *
 * @param planFile Path to the DICOM RT plan file.
 * @param side Either "Y1" or "Y2", specifying the MLC bank.
 * @param beamIdx Index of the beam.
 * @param controlpointIdx Index of the control point within the beam.
 * @return std::vector<G4double> Vector containing the MLC leaf positions for the specified side.
 *
 * @throws G4Exception If the side parameter is not "Y1" or "Y2".
 */
std::vector<G4double> IDicomPlan::ReadMlcPositioning(const std::string& planFile, const std::string& side, int beamIdx, int controlpointIdx){
  LOGSVC_INFO("DcmSvc","Reading the MLC configuration from {}",planFile);
  LOGSVC_INFO("DcmSvc","Side: {}, beamIdx: {}, controlpointIdx: {}",side,beamIdx,controlpointIdx);
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
 * @brief Reads the position of a specified jaw from a custom plan file.
 *
 * Searches for the "Jaws" header in the provided plan file, parses the subsequent line for jaw positions, and returns the value corresponding to the specified jaw name ("X1", "X2", "Y1", or "Y2").
 *
 * @param planFile Path to the custom plan file.
 * @param jawName Name of the jaw ("X1", "X2", "Y1", or "Y2") whose position is to be read.
 * @return double The position value of the specified jaw.
 *
 * @throws G4Exception if the file cannot be opened, the "Jaws" header is not found, or the jaw position line cannot be parsed.
 */
double ICustomPlan::ReadJawPossition(const std::string& planFile, const std::string& jawName, int beamIdx, int controlpointIdx) const{
  std::ifstream file(planFile);
  if (!file.is_open()) {
    G4String msg = "Could not open file: " + planFile;
    LOGSVC_FATAL("DcmSvc",msg.data());
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
    LOGSVC_FATAL("DcmSvc",msg.data());
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
      LOGSVC_FATAL("DcmSvc",msg.data());
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
 * @brief Reads the jaw aperture values for the specified side from a custom plan file.
 *
 * Retrieves the positions of the jaws ("X1" and "X2" for side "X", "Y1" and "Y2" for side "Y") for the given beam and control point indices from a custom plan file. Returns the pair of jaw positions in millimeters.
 *
 * @param side Specifies which jaw pair to read: "X" for X jaws, "Y" for Y jaws.
 * @return std::pair<double, double> The positions of the two jaws for the specified side.
 *
 * @note Exits the program if an unknown side is provided.
 */
std::pair<double,double> ICustomPlan::ReadJawsAperture(const std::string& planFile,const std::string& side,int beamIdx, int controlpointIdx){
  std::pair<double,double> jawsSideAperture;
  if(side=="X"){
    auto x1 = ReadJawPossition(planFile,"X1",beamIdx,controlpointIdx);
    auto x2 = ReadJawPossition(planFile,"X2",beamIdx,controlpointIdx);
    LOGSVC_INFO("DcmSvc","JAWS:: X1={} mm, X2={} mm",x1,x2);
    jawsSideAperture = std::make_pair(x1,x2);
  } else if(side=="Y"){
    auto y1 = ReadJawPossition(planFile,"Y1",beamIdx,controlpointIdx);
    auto y2 = ReadJawPossition(planFile,"Y2",beamIdx,controlpointIdx);
    LOGSVC_INFO("DcmSvc","JAWS:: Y1={} mm, Y2={} mm",y1,y2);
    jawsSideAperture = std::make_pair(y1,y2);
  } else {
      LOGSVC_ERROR("DcmSvc","ICustomPlan::ReadJawsAperture: Unknown side: {}", side);
      std::exit(EXIT_FAILURE);
  }
  AcknowledgeJawsAperture(side,jawsSideAperture);
  return std::move(jawsSideAperture);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Reads and returns the MLC leaf positions for a specified side from a custom plan file.
 *
 * Parses the custom plan file to extract and return the MLC (multi-leaf collimator) positions for either the "Y1" or "Y2" side, applying orientation correction as required by the file format. Throws a fatal exception if the side is invalid, the file cannot be opened, or the MLC header is not found.
 *
 * @param side Specifies which MLC side to read ("Y1" or "Y2").
 * @return std::vector<G4double> The MLC leaf positions for the requested side.
 */
std::vector<G4double> ICustomPlan::ReadMlcPositioning(const std::string& planFile, const std::string& side, int beamIdx, int controlpointIdx){
  
  LOGSVC_INFO("DcmSvc","Reading the MLC {} side configuration from {}",side,planFile);
  if(side!="Y1" && side!="Y2"){
    G4String msg = "Wrong input side given (" + side + ")! Allowed values are 'Y1' and 'Y2'";
    LOGSVC_FATAL("DcmSvc",msg.data());
    G4Exception("ICustomPlan", "GetMlcPositioning", FatalErrorInArgument, msg);
  }
  std::ifstream file(planFile);
  if (!file.is_open()) {
    G4String msg = "Could not open file: " + planFile;
    LOGSVC_FATAL("DcmSvc",msg.data());
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
    LOGSVC_FATAL("DcmSvc",msg.data());
    G4Exception("ICustomPlan", "GetMlcPositioning", FatalErrorInArgument, msg); 
  }
  while (std::getline(file, line)) {
    std::istringstream iss(line);
    std::string value_y1, value_y2;
    // Get the values as strings separated by a comma
    if (std::getline(iss, value_y1, ',') && std::getline(iss, value_y2)) {
      // Convert string to double and add to the respective vectors
      mlc_y2.push_back(-1*std::stod(value_y1)); // For propouse proper reading from custom dat and properly oriented this (MLC lew right side orientation.) we are using -1 here. 
      mlc_y1.push_back(-1*std::stod(value_y2)); // For propouse proper reading from custom dat and properly oriented this (MLC lew right side orientation.) we are using -1 here. 
    }
  }
  file.close();
  AcknowledgeMlcPositioning(side,side=="Y1" ? mlc_y1 : mlc_y2);
  return side=="Y1" ? std::move(mlc_y1) : std::move(mlc_y2);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Retrieves the gantry angle for a specific beam and control point from a DICOM RT plan.
 *
 * Imports the Python module `dicom_rtplan_angle` and calls its `return_angle_in_controlpoint` function to obtain the angle value for the given beam and control point indices.
 *
 * @param current_beam Index of the beam.
 * @param current_controlpoint Index of the control point within the beam.
 * @return double Gantry angle in degrees for the specified beam and control point.
 */
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
/**
 * @brief Retrieves the dose value for a specific beam and control point from a DICOM RT plan.
 *
 * Imports the Python module `dicom_rtplan_dose` and calls its `return_dose_during_specified_controlpoint` function to obtain the dose for the given beam and control point indices.
 *
 * @param current_beam Index of the beam.
 * @param current_controlpoint Index of the control point within the beam.
 * @return double Dose value for the specified beam and control point.
 */
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
/**
 * @brief Returns the number of beams in a DICOM RT plan file.
 *
 * Imports the Python module `dicom_rtplan_mlc` and calls its `return_number_of_beams` function to determine the total number of beams defined in the specified plan file.
 *
 * @param planFile Path to the DICOM RT plan file.
 * @return unsigned Number of beams in the plan.
 */
unsigned DicomSvc::GetRTPlanNumberOfBeams(const std::string& planFile) const {
  auto rtplanMlcReader = py::module::import("dicom_rtplan_mlc");
  auto beams_counter = rtplanMlcReader.attr("return_number_of_beams")(planFile);
  return beams_counter.cast<unsigned>();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the number of control points for a specified beam in a DICOM RT plan file.
 *
 * @param planFile Path to the DICOM RT plan file.
 * @param beamNumber Index of the beam for which to retrieve the control point count.
 * @return unsigned Number of control points for the specified beam.
 */
unsigned DicomSvc::GetRTPlanNumberOfControlPoints(const std::string& planFile,unsigned beamNumber) const{
  auto rtplanMlcReader = py::module::import("dicom_rtplan_mlc");
  return rtplanMlcReader.attr("return_number_of_controlpoints")(planFile,beamNumber).cast<unsigned>();
}

////////////////////////////////////////////////////////////////////////////////
/**
   * @brief Exports patient data to a CT series using the specified CSV and output path.
   *
   * Sets the output path for CT data and creates a CT series from the provided CSV file by delegating to the CT service.
   */
void DicomSvc::ExportPatientToCT(const std::string& series_csv_path, const std::string& output_path) const {

    // PyGILState_Release(gstate);
    m_ct_svc.set_paths(output_path);
    m_ct_svc.create_ct_series(series_csv_path);
  }

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Retrieves the control point configuration for a given ID and plan file.
 *
 * Ensures the service is initialized and the plan file path is absolute before delegating to the underlying plan's configuration retrieval.
 *
 * @param id Identifier of the control point.
 * @param planFile Path to the plan file (relative or absolute).
 * @return ControlPointConfig The configuration for the specified control point.
 */
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
/**
 * @brief Returns a control point configuration for a DICOM RT plan.
 *
 * Constructs and returns a `ControlPointConfig` object with preset field sizes and type "RTPlan". The configuration is associated with the provided plan file and identifier.
 *
 * @param id Identifier for the control point.
 * @param planFile Path to the DICOM RT plan file.
 * @return ControlPointConfig The constructed control point configuration.
 */
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
/**
 * @brief Retrieves the control point configuration for a custom plan file.
 *
 * Extracts the number of events and rotation from the specified custom plan file and constructs a ControlPointConfig object with these values and additional fixed field parameters.
 *
 * @param id Identifier for the control point.
 * @param planFile Path to the custom plan file.
 * @return ControlPointConfig Populated configuration for the specified control point.
 */
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
/**
 * @brief Extracts the number of particles from a custom plan file.
 *
 * Searches the specified plan file for a line starting with "# Particles:" and returns the numeric value following the colon. Returns 0 if the line is not found or cannot be parsed.
 *
 * @param planFile Path to the custom plan file.
 * @return int Number of particles specified in the plan file, or 0 if not found.
 */
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
/**
 * @brief Extracts the rotation value from a custom plan file.
 *
 * Searches the specified plan file for a line starting with "# Rotation:" and returns the numeric value following the colon. Returns 0.0 if the rotation value is not found.
 *
 * @param planFile Path to the custom plan file.
 * @return double Rotation value in degrees, or 0.0 if not specified.
 */
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
 * @brief Retrieves the Hounsfield unit value for a given material from a configuration file.
 *
 * Looks up the Hounsfield unit (HU) value for the specified material name in the `hounsfield_scale_120keV.toml` configuration file. If `normalized` is true, the value is linearly scaled between 0.02 (Vacuum) and 0.98 (Tungsten). Returns 0 if the material is not found.
 *
 * @param materialName Name of the material to look up.
 * @param normalized If true, returns the HU value normalized between 0.02 and 0.98.
 * @return double The Hounsfield unit value for the material, optionally normalized.
 */
double DicomSvc::GetHounsfieldScaleValue(const std::string& materialName, bool normalized){
  static std::unique_ptr<toml::table> tconfig;
  if(!tconfig){
    auto hausfieldMaterialMapFile = std::string(PROJECT_DATA_PATH) + "/config/hounsfield_scale_120keV.toml";
    tconfig = std::make_unique<toml::table>(toml::parse_file(hausfieldMaterialMapFile));
    LOGSVC_INFO("DcmSvc","Reading from Hounsfield Material Map File: {}",hausfieldMaterialMapFile);
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


