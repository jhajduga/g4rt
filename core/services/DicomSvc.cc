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
 * @brief Configure paths used by the Python CT service.
 *
 * Sets the CT service output directory to the provided path and configures the service's project path to the build-time project data location.
 *
 * @param output_path Path to the directory where CT output files will be written.
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
 * @brief Selects and constructs the appropriate plan handler for the given plan file type.
 *
 * Creates an ICt plan handler: constructs an ICustomPlan when the file type is "dat" or an IDicomPlan when the file type is "dcm".
 *
 * @param planFileType The plan file extension identifier ("dat" or "dcm").
 * @throws G4Exception if the file type is unsupported.
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
 * @brief Update the active radiotherapy plan file and select the corresponding plan handler.
 *
 * Sets the service's current plan file path and initializes the plan implementation based on the file's extension
 * (for example: "dat" -> custom plan, "dcm" -> DICOM plan).
 *
 * @param plan_file Path to the RT plan file; its extension determines which plan handler is created.
 */
void DicomSvc::SetPlanFile(const std::string& plan_file){
  m_rtplan_file = plan_file;
  Initialize(svc::getFileExtenstion(m_rtplan_file));
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Compute the total number of control points across all beams in the current RT plan.
 *
 * @return Total count of control points summed over all beams.
 */
unsigned DicomSvc::GetTotalNumberOfControlPoints() const {
  return std::accumulate(m_rtplan_beam_n_control_points.begin(), m_rtplan_beam_n_control_points.end(), 0);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Accessor for the application's single DicomSvc instance.
 *
 * Ensures a single shared DicomSvc is created and lives for the remainder of the process.
 *
 * @return DicomSvc* Pointer to the global DicomSvc singleton.
 */
DicomSvc *DicomSvc::GetInstance() {
  static DicomSvc instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Retrieve the position of a specified jaw for a given beam and control point in a DICOM RT plan file.
 *
 * @param planFile Path to the DICOM RT plan file.
 * @param jawName Name of the jaw; one of "X1", "X2", "Y1", or "Y2" (case-sensitive).
 * @param beamIdx Zero-based index of the beam.
 * @param controlpointIdx Zero-based index of the control point within the beam.
 * @return double Position of the specified jaw in the plan's coordinate units.
 *
 * @throws G4Exception If an invalid `jawName` is provided.
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
 * @brief Retrieve the two jaw positions for a specified side from an RT plan.
 *
 * Reads and returns the pair of jaw positions for the requested side: for side "X" returns (X1, X2); for side "Y" returns (Y1, Y2).
 *
 * @param planFile Path to the DICOM RT plan file.
 * @param side Jaw side identifier, either "X" (X1/X2) or "Y" (Y1/Y2).
 * @param beamIdx Zero-based beam index.
 * @param controlpointIdx Zero-based control point index within the beam.
 * @return std::pair<double,double> Pair of jaw positions: (X1, X2) when side == "X", (Y1, Y2) when side == "Y".
 *
 * @note The function expects `side` to be exactly "X" or "Y"; behavior is undefined for other values.
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
 * @brief Validate MLC leaf positions for the specified side.
 *
 * Verify that the provided leaf-position vector is well-formed and complete for the MLC side identified by `side`. If the data is missing, has unexpected length, or otherwise fails validation, a runtime exception is raised.
 *
 * @param side Identifier of the MLC side (e.g., "Y1" or "Y2").
 * @param mlc_positioning Ordered leaf positions for the specified side.
 * @throws G4Exception If the positioning data is invalid or incomplete.
 */
void IPlan::AcknowledgeMlcPositioning(const std::string& side, const std::vector<G4double>& mlc_positioning) const {
  // TODO
  // Check the MLC type and verify if complete and correct information is read-in;
  // if not, throw an exception
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Retrieve MLC leaf positions for a specified MLC bank, beam, and control point from a DICOM RT plan.
 *
 * @param planFile Path to the DICOM RT plan file.
 * @param side MLC bank identifier: "Y1" or "Y2".
 * @param beamIdx Index of the beam.
 * @param controlpointIdx Index of the control point within the beam.
 * @return std::vector<G4double> Vector of leaf positions for the specified side, ordered by leaf index.
 *
 * @throws G4Exception If `side` is not "Y1" or "Y2".
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
 * @brief Return the position of a named jaw from a custom plan file.
 *
 * Searches the plan file for a "Jaws" header, parses the following comma-separated jaw line, and returns the numeric position for the requested jaw.
 *
 * @param planFile Path to the custom plan file to read.
 * @param jawName Jaw identifier to retrieve: "X1", "X2", "Y1", or "Y2".
 * @param beamIdx Beam index (present for interface compatibility; not used for custom plan parsing).
 * @param controlpointIdx Control-point index (present for interface compatibility; not used for custom plan parsing).
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
 * @brief Read jaw aperture positions for a specified side from a custom plan file.
 *
 * Reads the two jaw positions for the given beam and control point from the custom plan file.
 *
 * @param planFile Path to the custom plan file.
 * @param side Which jaw pair to read: "X" for X1/X2, "Y" for Y1/Y2.
 * @param beamIdx Beam index to query.
 * @param controlpointIdx Control point index to query.
 * @return std::pair<double,double> First and second jaw positions (in millimeters) for the specified side.
 *
 * @note The process terminates if `side` is not "X" or "Y".
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
 * @brief Retrieve MLC leaf positions for the specified side from a custom plan file.
 *
 * Parses the plan file to extract multi-leaf collimator positions for the "Y1" or "Y2" side and returns them with the file-format-required orientation correction applied.
 *
 * @param planFile Path to the custom plan file to read.
 * @param side Which MLC side to return; allowed values are "Y1" or "Y2".
 * @param beamIdx Beam index associated with the query (kept for interface consistency).
 * @param controlpointIdx Control point index associated with the query (kept for interface consistency).
 * @return std::vector<G4double> The requested side's MLC leaf positions.
 *
 * @throws G4Exception if `side` is not "Y1" or "Y2", if the file cannot be opened, or if an "MLC" header is not found in the file.
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
 * @brief Get the gantry angle for a specific beam and control point.
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
 * @brief Retrieves the dose for the specified beam and control point from the loaded RT plan.
 *
 * @param current_beam Index of the beam within the RT plan.
 * @param current_controlpoint Index of the control point within the specified beam.
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
 * @brief Determine the total number of beams in a DICOM RT plan file.
 *
 * @param planFile Path to the DICOM RT plan file.
 * @return unsigned Total number of beams defined in the plan.
 */
unsigned DicomSvc::GetRTPlanNumberOfBeams(const std::string& planFile) const {
  auto rtplanMlcReader = py::module::import("dicom_rtplan_mlc");
  auto beams_counter = rtplanMlcReader.attr("return_number_of_beams")(planFile);
  return beams_counter.cast<unsigned>();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Retrieve the number of control points for a specific beam in a DICOM RT plan file.
 *
 * @param planFile Path to the DICOM RT plan file.
 * @param beamNumber Index of the beam within the plan.
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
 * @brief Create a control point configuration for a DICOM RT plan.
 *
 * Constructs a ControlPointConfig associated with the given plan file and identifier,
 * populating the config with preset field sizes and the field type "RTPlan".
 *
 * @param id Identifier for the control point.
 * @param planFile Path to the DICOM RT plan file associated with the configuration.
 * @return ControlPointConfig The constructed control point configuration with populated fields.
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
 * @brief Reads a rotation value from a custom plan file.
 *
 * Searches the file for a line beginning with "# Rotation:" and parses the numeric value following the colon.
 *
 * @param planFile Path to the custom plan file.
 * @return double Rotation in degrees parsed from the file, or 0.0 if no rotation line is present.
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
 * @brief Obtain the Hounsfield unit (HU) value for a named material from the project's TOML mapping.
 *
 * If the named material exists under the `[Hounsfield]` table in the configured
 * `hounsfield_scale_120keV.toml`, its HU value is returned. When `normalized` is
 * true, the HU is linearly mapped to the range [0.02, 0.98] using the file's
 * `Vacuum` and `Tungsten` entries as the source minimum and maximum.
 *
 * @param materialName Material key to look up in the `[Hounsfield]` table.
 * @param normalized When true, return the HU value normalized to [0.02, 0.98].
 * @return double The HU value for `materialName`, or 0 if the material is not present.
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

