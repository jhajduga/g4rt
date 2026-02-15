#include "GeoSvc.hh"
#include "Services.hh"
#include "WorldConstruction.hh"
#include <iostream>
#include <sstream>
#include "G4Run.hh"
#include "D3DDetector.hh"
#include "PatientGeometry.hh"
#include "LinacGeometry.hh"
#include "VPatient.hh"
#include "BeamCollimation.hh"
#include "colors.hh"
#include <TGeoManager.h>
#include <TFile.h>
#include "IO.hh"
#include "G4GDMLParser.hh"

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs the GeoSvc singleton and initializes its configuration.
 *
 * Sets up the geometry service by invoking the configuration routine upon creation.
 */
GeoSvc::GeoSvc() : TomlConfigurable("GeoSvc"){
  Configure();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Destructor for the GeoSvc service.
 *
 * Unregisters the service configuration upon destruction.
 */
GeoSvc::~GeoSvc() {
  configSvc()->Unregister(thisConfig()->GetName());
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Provides access to the global GeoSvc singleton instance.
 *
 * @return Pointer to the unique GeoSvc singleton.
 */
GeoSvc *GeoSvc::GetInstance() {
  static GeoSvc instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Checks if the geometry world has been constructed.
 *
 * @return true if the geometry world instance exists; false otherwise.
 */
bool GeoSvc::IsWorldBuilt() const { return my_world ? true : false; }

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Defines and registers configuration parameters for the geometry service.
 *
 * Sets up all configurable units related to geometry construction, export options, and model selection, then applies their default values.
 */
void GeoSvc::Configure() {
  //G4cout << "[INFO]:: GeoSvc :: Service default configuration " << G4endl;

  DefineUnit<G4String>("HeadModel");
  DefineUnit<std::string>("MlcModel");
  DefineUnit<G4double>("isoCentre");
  DefineUnit<std::string>("SavePhspRequest");
  DefineUnit<VecG4doubleSPtr>("SavePhSpHead");
  DefineUnit<VecG4doubleSPtr>("SavePhSpUsr");
  DefineUnit<G4bool>("BuildLinac");
  DefineUnit<G4bool>("BuildPatient");

  DefineUnit<bool>("ExportGeometryToTFile");
  DefineUnit<bool>("ExportGeometryToGDML");
  DefineUnit<bool>("ExportGeometryToGate");
  DefineUnit<bool>("ExportScoringGeometryToCsv");

  DefineUnit<std::string>("GeoDir");


  Configurable::DefaultConfig();   // setup the default configuration for all defined units/parameters
  // Configurable::PrintConfig();

}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Sets the default value for a specified geometry configuration parameter.
 *
 * Assigns a default value to the given configuration unit, such as head model, MLC model, isocentre distance, phase space plane requests, build flags, export options, or geometry directory. Used to initialize or reset configuration parameters to their standard defaults.
 *
 * @param unit The name of the configuration parameter to set to its default value.
 */
void GeoSvc::DefaultConfig(const std::string &unit) {

  // Config volume name
  if (unit.compare("Label") == 0){
    // G4cout << "[DEBUG]:: GeoSvc::DefaultConfig:   " << unit << G4endl;
    m_config->SetValue(unit, std::string("Geometry Service"));
    // G4cout << "[DEBUG]:: GeoSvc::DefaultConfig value seted:  " << unit << G4endl;
  }

  if (unit.compare("HeadModel") == 0){
    // G4cout << "[DEBUG]:: GeoSvc::DefaultConfig:   " << unit << G4endl;
    m_config->SetValue(unit, G4String("BeamCollimation"));
    // G4cout << "[DEBUG]:: GeoSvc::DefaultConfig value seted:  " << unit << G4endl;
  }

  if (unit.compare("MlcModel") == 0){
    // G4cout << "[DEBUG]:: GeoSvc::DefaultConfig:   " << unit << G4endl;
    // m_config->SetValue(unit, G4String("Varian-HD120")); 
    thisConfig()->SetTValue<std::string>(unit, std::string("None")); 
    // G4cout << "[DEBUG]:: GeoSvc::DefaultConfig value seted:  " << unit << G4endl;
  }

  //  Distance between the isocentre and the target of the accelerator
  if (unit.compare("isoCentre") == 0){
    // G4cout << "[DEBUG]:: GeoSvc::DefaultConfig:   " << unit << G4endl;
    m_config->SetValue(unit, G4double(1000.));  // mm
    // G4cout << "[DEBUG]:: GeoSvc::DefaultConfig value seted:  " << unit << G4endl;
  }

  // See note in GeoSvc::ParseSavePhspPlaneRequest
  if (unit.compare("SavePhspRequest") == 0){
    // G4cout << "[DEBUG]:: GeoSvc::DefaultConfig:   " << unit << G4endl;
    m_config->SetValue(unit, std::string("Usr:0."));
    // G4cout << "[DEBUG]:: GeoSvc::DefaultConfig value seted:  " << unit << G4endl;
  }

  if (unit.compare("SavePhSpHead") == 0){
    // G4cout << "[DEBUG]:: GeoSvc::DefaultConfig:   " << unit << G4endl;
    m_config->SetValue(unit, std::make_shared<VecG4double>());
    // G4cout << "[DEBUG]:: GeoSvc::DefaultConfig value seted:  " << unit << G4endl;
  }

  if (unit.compare("SavePhSpUsr") == 0) {
    // G4cout << "[DEBUG]:: GeoSvc::DefaultConfig:   " << unit << G4endl;
    m_config->SetValue(unit, std::make_shared<VecG4double>());
    auto vecPtr = m_config->GetValue<VecG4doubleSPtr>(unit);
    vecPtr->emplace_back(2 * nm);
    // G4cout << "[DEBUG]:: GeoSvc::DefaultConfig value seted:  " << unit << G4endl;
  }

  if (unit.compare("BuildLinac") == 0){
    // G4cout << "[DEBUG]:: GeoSvc::DefaultConfig:   " << unit << G4endl;
      thisConfig()->SetTValue<bool>(unit, G4bool(true));
    // G4cout << "[DEBUG]:: GeoSvc::DefaultConfig value seted:  " << unit << G4endl;
  }
  if (unit.compare("BuildPatient") == 0){
    // G4cout << "[DEBUG]:: GeoSvc::DefaultConfig:   " << unit << G4endl;
     thisConfig()->SetTValue<bool>(unit, G4bool(true));
    // G4cout << "[DEBUG]:: GeoSvc::DefaultConfig value seted:  " << unit << G4endl;
  }

  if (unit.compare("ExportGeometryToTFile") == 0)
    thisConfig()->SetTValue<bool>(unit, false);

  if (unit.compare("ExportGeometryToGDML") == 0)
    thisConfig()->SetTValue<bool>(unit, false);

  if (unit.compare("ExportGeometryToGate") == 0)
    thisConfig()->SetTValue<bool>(unit, false);

  if (unit.compare("ExportScoringGeometryToCsv") == 0)
    thisConfig()->SetTValue<bool>(unit, false);

  if (unit.compare("GeoDir") == 0)
      thisConfig()->SetTValue<std::string>(unit, std::string("geo"));
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Initializes the geometry service and validates configuration.
 *
 * Performs one-time initialization by printing the current configuration, parsing phase space plane requests if enabled, and verifying the configured head and MLC models. Marks the service as initialized to prevent repeated setup.
 */
void GeoSvc::Initialize() {
  if (!m_isInitialized) {
    
      INFO_GEO("Service initialization...");
    PrintConfig();

    if (m_configSvc->GetValue<bool>("RunSvc", "SavePhSp")) 
      ParseSavePhspPlaneRequest();

    GetHeadModel(); // verify the specified head model
    GetMlcModel();  // verify the specified MLC model

    m_isInitialized = true;
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Updates the geometry world for the specified run ID.
 *
 * Attempts to update the existing geometry world instance with the provided run ID. Returns the world instance regardless of update success.
 *
 * @param runId The identifier for the current run, used to update geometry state.
 * @return WorldConstruction* Pointer to the updated geometry world instance.
 */
WorldConstruction *GeoSvc::Update(int runId) {
  G4cout << "[INFO]:: GeoSvc :: Geometry world is already existing - Updating.." << G4endl;
  if (my_world->Update(runId)) 
    return my_world;
  else {
    G4cout << "[ERROR]:: GeoSvc :: Failed with geometry update" << G4endl;
    return my_world;
  }

}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Ensure the service is initialized and construct the geometry world if absent.
 *
 * Initializes the service when necessary and creates the singleton WorldConstruction instance if one does not already exist.
 *
 * @return Pointer to the constructed or existing WorldConstruction instance.
 */
WorldConstruction *GeoSvc::Build() {

  if (!m_isInitialized) { Initialize(); }
  if (!my_world) {
    Initialize();
    G4cout << "[INFO]:: GeoSvc :: Building geometry world" << G4endl;
    my_world = WorldConstruction::GetInstance();
  } else {
    G4cout << "[INFO]:: GeoSvc :: Geometry world is already exists" << G4endl;
  }
  return my_world;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Destroy the current geometry world and free its resources.
 *
 * If a geometry world exists, it is destroyed and the service's reference to it is cleared; if no world exists, the call has no effect.
 */
void GeoSvc::Destroy() {
  G4cout << "[INFO]:: GeoSvc :: destroying geometry world" << G4endl;
  if (my_world) {
    my_world->Destroy();
    my_world = nullptr;
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Parses and validates the phase space plane save request from configuration.
 *
 * Interprets the `SavePhspRequest` configuration string to determine where phase space planes should be saved during simulation. Supports both predefined head positions (before specific linac components) and user-defined positions relative to the isocenter. Updates the corresponding configuration vectors with the requested positions, enforcing that only one phase space plane can be saved per run. Throws a fatal Geant4 exception if the request is invalid, unsupported, or out of allowed range.
 */
void GeoSvc::ParseSavePhspPlaneRequest() {
  // NOTE:
  // Two the different types phsp planes are defined.
  // Usr specify next positions values following the colon after 'Head' and being semicolon separated.
  // HEAD POSITIONING:
  //    1. Before primary collimator lower
  //    2. Before flattening filter
  //    3. Before jaws
  //    4. Before MLC
  // USER POSITIONING:
  //       Number of phsp can be defined in the range 0 - 45 cm, where 0 cm
  //       is at the isocentre (phantom surface), and 45 cm is up to the target direction.
  //       Usr specify next positions values following the colon after 'Usr' and being semicolon separated.
  //
  // NOTE: Currently there is a limit to one phsp to be generated in single run.
  //
  // An example requests, run with run_tui
  // ./run_tui -g --SavePhSpace --PhspPlanePos "Head:4"
  // ./run_tui -g --SavePhSpace --PhspPlanePos "Usr:15.5"

  std::string savePhspRequest = m_configSvc->GetValue<std::string>("GeoSvc", "SavePhspRequest");
  G4cout << "[INFO]:: GeoSvc :: ParseSavePhspPlaneRequest: \"" << savePhspRequest << "\"" << G4endl;

  auto saveHeadPhsp = m_configSvc->GetValue<VecG4doubleSPtr>("GeoSvc", "SavePhSpHead");
  auto saveUsrPhsp = m_configSvc->GetValue<VecG4doubleSPtr>("GeoSvc", "SavePhSpUsr");

  if (G4String(savePhspRequest).compare("Usr:0.") != 0) { // differ from default value

    // Clear vectors (of default positions) when SavePhspRequest string is not empty
    saveHeadPhsp->clear();
    saveUsrPhsp->clear();

    // get rid of empty cells from the string value
    std::remove_if(savePhspRequest.begin(), savePhspRequest.end(), [](unsigned char x) { return std::isspace(x); });

    std::size_t headPhsp = savePhspRequest.find("Head");
    std::size_t usrPhsp = savePhspRequest.find("Usr");

    //  FIX POSITIONED phsp is implemented only for 2300CD geometry.
    G4String geometry = "TrueBeamMockup"; // TODO: temporary, to be refactored
    if (headPhsp != std::string::npos && geometry.compare("Clinac2300cd") != 0) {
      G4String description = "You have requested to save the phsp within the head world.";
      description += " This is implemented only for Varian CLinac 2300CD geometry";
      G4Exception("GeoSvc", "ParseSavePhspPlaneRequest", FatalErrorInArgument, description.data());
    }

    std::vector<std::string> val_usr_str_vect;
    std::string headPhsp_str, usrPhsp_str;
    if (headPhsp != std::string::npos && usrPhsp == std::string::npos) {        // HEAD POSITIONING
      headPhsp_str = savePhspRequest.substr(5, savePhspRequest.length()); // Head:...
    } else if (headPhsp == std::string::npos && usrPhsp != std::string::npos) { // USER POSITIONING
      usrPhsp_str = savePhspRequest.substr(4, savePhspRequest.length()); // Usr:...
      std::istringstream ssUsr(usrPhsp_str);
      std::string val_str;
      while (std::getline(ssUsr, val_str, ';')) val_usr_str_vect.push_back(val_str);
    } else if (headPhsp != std::string::npos && usrPhsp != std::string::npos) {   // HEAD & USER POSITIONING
      if (headPhsp < usrPhsp) {
        headPhsp_str = savePhspRequest.substr(5, usrPhsp - 6);                        // Head:...
        usrPhsp_str = savePhspRequest.substr(usrPhsp + 4, savePhspRequest.length());  // Usr:...
      } else { // inverted order
        usrPhsp_str = savePhspRequest.substr(4, headPhsp - 6);                          // Usr:...
        headPhsp_str = savePhspRequest.substr(headPhsp + 5, savePhspRequest.length());  // Head:...
      }
      std::istringstream ssUsr(usrPhsp_str);
      std::string val_str;
      while (std::getline(ssUsr, val_str, ';')) val_usr_str_vect.push_back(val_str);


    }

    if (!headPhsp_str.empty()) { // HEAD POSITIONING
      // NOTE: temporary documentation:
      // https://docs.google.com/presentation/d/1meup9cPSCJjbhjClDrVJ0cyAwgN5QMQiMHn5vwJfrBU/edit#slide=id.g3fb4c4ac7e_3_43
      if (headPhsp_str.find("1") != std::string::npos) {
        G4cout << "[INFO]:: SavePhsp Head:: 1 - Write phsp above the primary collimator lower" << G4endl;
        saveHeadPhsp->emplace_back(-97.45 * cm);
      }
      if (headPhsp_str.find("2") != std::string::npos) {
        G4cout << "[INFO]:: SavePhsp Head:: 2 - Write phsp above the  flattening filter" << G4endl;
        saveHeadPhsp->emplace_back(-89.75 * cm);
      }
      if (headPhsp_str.find("3") != std::string::npos) {
        G4cout << "[INFO]:: SavePhsp Head:: 3 - Write phsp above the jaws" << G4endl;
        saveHeadPhsp->emplace_back(-78.5 * cm);
      }
      if (headPhsp_str.find("4") != std::string::npos) {
        G4cout << "[INFO]:: SavePhsp Head:: 4 - Write phsp above the MLC" << G4endl;
        saveHeadPhsp->emplace_back(-57.5 * cm);
      }
    }

    if (!usrPhsp_str.empty()) { // USER POSITIONING
      G4cout << "[INFO]:: SavePhsp User:: Write phsp X(" << usrPhsp_str << ") cm above the isocentre." << G4endl;
      for (auto &is : val_usr_str_vect) { // vector of already splitted values
        saveUsrPhsp->push_back(G4double(-std::stod(is) * cm)); // convert to the actual world coordinate system
        if (saveUsrPhsp->back() > 0 || saveUsrPhsp->back() < -39.5 * cm) {
          G4String description = "You can request to save the phsp in a range of [0 cm ,39.5 cm] above iso-centre";
          G4Exception("GeoSvc", "ParseSavePhspPlaneRequest", FatalErrorInArgument, description.data());
        }
        if (saveUsrPhsp->back() == 0. * cm) {
          saveUsrPhsp->back() += 2. * nm;
          G4cout << "[WARNING]:: Phsp position of usr request at 0cm has been shifted by 2nm" << G4endl;
        }
      }
    }
    // Limit to one phsp to be generated in single run - to be removed in future(?), needs debugging!
    if (saveHeadPhsp->size() + saveUsrPhsp->size() > 1) {
      G4String description = "You can request to save one phsp in a single run.";
      G4Exception("GeoSvc", "ParseSavePhspPlaneRequest", FatalErrorInArgument, description.data());
    }
  } else {
    G4cout << "[WARNING]:: SavePhsp performed with default positioning." << G4endl;
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Get the configured head geometry model.
 *
 * Reads the GeoSvc head model configuration and returns the corresponding EHeadModel value.
 *
 * @return EHeadModel The selected head geometry model.
 *
 * @note If the configured head model name is unrecognized, the process exits with status 0.
 */
EHeadModel GeoSvc::GetHeadModel() const {
  auto headName = m_configSvc->GetValue<G4String>("GeoSvc", "HeadModel");
  if (headName.compare("BeamCollimation") == 0) {
    return EHeadModel::BeamCollimation;
  } else if (headName.compare("None") == 0) {
    return EHeadModel::None;
  }else {
    G4cout << "[ERROR] Whoops, specified head geometry model (\""<<headName<<"\") not found (project dumped) " << G4endl;
    exit(0);
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Map the configured "MlcModel" string to the corresponding EMlcModel value.
 *
 * Reads the "MlcModel" setting from GeoSvc configuration and returns the matching enum value. If the configured name is not recognized the process is terminated.
 *
 * @return EMlcModel The selected MLC model.
 */
EMlcModel GeoSvc::GetMlcModel() const {
  auto mlcName = m_configSvc->GetValue<std::string>("GeoSvc", "MlcModel");
  if (mlcName.compare("Varian-Millennium") == 0) {
    return EMlcModel::Millennium;
  } else if (mlcName.compare("Varian-HD120") == 0) {
    return EMlcModel::HD120;
  } else if (mlcName.compare("Simplified") == 0) {
    return EMlcModel::Simplified;
  } else if (mlcName.compare("None") == 0) {
    return EMlcModel::None;
  }else {
    G4cout << "[ERROR] Whoops, specified mlc model (\""<<mlcName<<"\") not found (project dumped) " << G4endl;
    exit(0);
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Retrieve the patient geometry object from the service or world.
 *
 * Returns the cached patient object if set; otherwise queries the current world's
 * patient environment and returns its patient. If no patient is available, returns nullptr.
 *
 * @return VPatient* Pointer to the patient object if available, `nullptr` otherwise.
 */
VPatient* GeoSvc::Patient(){
  if (m_patient)
    return m_patient;
  if(World()->PatientEnvironment())
    return World()->PatientEnvironment()->GetPatient();
  return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Retrieve custom detector objects present in the geometry world.
 *
 * @return std::vector<VPatient*> Vector of pointers to custom detector instances; empty if none are present.
 */
std::vector<VPatient*> GeoSvc::CustomDetectors(){
  return World()->GetCustomDetectors();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Accesses the configured multi-leaf collimator (MLC) instance.
 *
 * @return Pointer to the MLC instance when a head model is configured, `nullptr` otherwise.
 */
VMlc* GeoSvc::MLC(){
  if(thisConfig()->GetValue<G4String>("HeadModel")){
    return BeamCollimation::GetInstance()->GetMlc();
  } 
  return nullptr; // this should never happen, but prevent warning
}


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Registers a scoring component for geometry export and analysis.
 *
 * Adds the specified scoring component to the internal list for later export or processing.
 *
 * @param element Pointer to the scoring component to register.
 */
void GeoSvc::RegisterScoringComponent(const GeoComponet *element){
  m_scoring_components.emplace_back(element);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Export the constructed geometry world to a ROOT TFile using the service's default export configuration.
 *
 * Writes the current geometry (including visualization settings applied by the service) into a ROOT TFile placed in the configured output directory.
 */
void GeoSvc::PerformDefaultExport() {
  WriteWorldToTFile();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Performs geometry and scoring component exports based on configuration flags.
 *
 * Conditionally exports the world geometry and scoring components to various formats, including ROOT TFile, CSV, GATE, and GDML, according to the current configuration settings.
 */
void GeoSvc::PerformRequestedExport() {
  if(thisConfig()->GetValue<bool>("ExportGeometryToTFile") && !m_is_tfile_exported)
    WriteWorldToTFile();
  if(thisConfig()->GetValue<bool>("ExportScoringGeometryToCsv"))
    WriteScoringComponentsPositioningToCsv();
  if(thisConfig()->GetValue<bool>("ExportGeometryToGate"))
    ExportToGateGenericRepeater();
  if(thisConfig()->GetValue<bool>("ExportGeometryToGDML"))
    WriteWorldToGdml();
}


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Exports all registered scoring components to GATE-compatible CSV files.
 *
 * Each scoring component is exported to a CSV file in the output directory for use with GATE simulations.
 */
void GeoSvc::ExportToGateGenericRepeater() const {
  std::cout << "[INFO] GeoSvc:: ExportToGateGenericRepeater..." <<std::endl;
  std::string output_dir = Service<ConfigSvc>()->GetValue<std::string>("RunSvc","OutputDir");
  for(const auto& gc : m_scoring_components){
      // GATE like export
      gc->ExportToGateCsv(output_dir);
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Exports the positioning data of all registered scoring components to CSV files.
 *
 * For each registered scoring component, writes both cell and voxel positioning information to CSV files in the output directory.
 */
void GeoSvc::WriteScoringComponentsPositioningToCsv() const {
  
      INFO_GEO("Writing Scroing Components to CSV...");
  std::string output_dir = GetOutputDir();
  for(const auto& gc : m_scoring_components){
      // Generic geometry export
      gc->ExportCellPositioningToCsv(output_dir);
      // Voxelised geometry export
      gc->ExportVoxelPositioningToCsv(output_dir);
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Placeholder for exporting scoring components' positioning data to a ROOT TFile.
 *
 * This function is not yet implemented. Intended to export the positioning information of all registered scoring components to a ROOT TFile in the output directory.
 */
void GeoSvc::WriteScoringComponentsPositioningToTFile() const {
  
      INFO_GEO("Writing Scroing Components to TFile...");
  std::string output_dir = GetOutputDir();
  for(const auto& gc : m_scoring_components){
      
      INFO_GEO("Implement me ...");
      // gc->ExportPositioningToTFile(output_dir); to be repaired to new scoring maps scheme
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Exports the world geometry to a ROOT TFile as a TGeoManager object.
 *
 * Converts the GDML representation of the world geometry into a ROOT TGeoManager, applies visualization settings for materials and components, and writes the geometry to a ROOT file in the output directory. Ensures GDML export is performed before conversion if necessary.
 */
void GeoSvc::WriteWorldToTFile() {
  
      DEBUG_GEO("Writing World Geometry To TFile...");
  auto output_dir = GetOutputDir();
  if(!m_is_gdml_exported){
    WriteWorldToGdml();
  }
  std::string geo_gdml_file = output_dir+"/"+m_world_file_name+".gdml";
  auto geo_tfile = output_dir+"/"+m_world_file_name+".root";
  auto tf = std::make_unique<TFile>(geo_tfile.c_str(),"RECREATE");
  auto geo_dir = tf->mkdir("Geometry");
  TGeoManager::SetDefaultUnits(TGeoManager::kG4Units);
  auto tgeom = TGeoManager::Import(geo_gdml_file.c_str());
  tgeom->SetTitle("G4RT World Geometry");
  tgeom->LockGeometry();
  auto tgeo = tgeom->GetTopVolume();
  tgeo->SetVisContainers(true);

  // Lambda to set colour to TGeoVolume
  // colour = -1 : SetVisibility FALSE
  auto setTGeoVolumeVis = [](TGeoVolume* gv, Int_t colour, Int_t transp){
    if(colour>=0){
      gv->SetLineColor(colour);
      gv->SetFillColor(colour);
      gv->SetTransparency(transp);
    } else {
      gv->SetVisibility(false);
    }
  };

  // Lambda to set visibility depending on name
  auto setNodesVisByName = [&](const TString& containStr, Int_t colour, Int_t transp=0){
    TGeoIterator next(tgeom->GetMasterVolume());
    TGeoNode *node;
    while ((node = next())) {
      if ( G4StrUtil::contains(node->GetVolume()->GetName(), containStr.Data())) {
        setTGeoVolumeVis(node->GetVolume(),colour,transp);
      }
    }
  };

  // Lambda to set visibility depending on medium
  auto setNodesVisByMaterial = [&](const TString& material, Int_t colour, Int_t transp=0){
    TGeoIterator next(tgeom->GetMasterVolume());
    TGeoNode *node;
    while ((node = next())) {
      if (G4StrUtil::contains(node->GetVolume()->GetMaterial()->GetName(), material.Data())) {
        setTGeoVolumeVis(node->GetVolume(),colour,transp);
      }
    }
  };

  // Environment visibility
  setNodesVisByMaterial("G4_Galactic",-1);
  setNodesVisByMaterial("G4_WATER",38,50);
  setNodesVisByMaterial("BaritesConcrete",12,50);
  // setNodesVisByMaterial("PMMA",12,30);
  setNodesVisByMaterial("PMMA",95,90);
  setNodesVisByMaterial("PLA",57,70);
  setNodesVisByMaterial("RW3",83,30);
  setNodesVisByMaterial("Rubber",156,30);

  // Dose3D visibility 
  setNodesVisByName("D3D",49);
  setNodesVisByName("Stl",12,10);

  // Final export
  geo_dir->WriteTObject(tgeom,"World_Geometry");
  tgeom->UnlockGeometry();
  
      INFO_GEO("Writing to {} - done!",geo_tfile);
  m_is_tfile_exported = true;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Exports the patient geometry CT data to CSV files in the output directory.
 *
 * The exported files are saved under a "dicom/ct_csv" subdirectory within the geometry output directory.
 */
void GeoSvc::WritePatientToCsvCT(){
  auto output_dir = GetOutputDir()+"/dicom/ct_csv";
  PatientGeometry::GetInstance()->ExportToCsvCT(output_dir);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Convert exported patient CT CSV files into DICOM CT files.
 *
 * Reads CSV-formatted patient CT data from the service output directory's "dicom/ct_csv"
 * subdirectory and writes resulting DICOM CT files to "dicom/ct_dcm" under the same
 * output directory by delegating to the DicomSvc.
 */
void GeoSvc::WritePatientToDicomCT(){
  // NOTE: Currently this service is using the csv data, 
  //       hence the GeoSvc::WritePatientToCsvCT has to be called!
  auto output_dir = GetOutputDir()+"/dicom/ct_csv";
  auto dicomSvc = Service<DicomSvc>();
  auto dciom_dir = GetOutputDir()+"/dicom/ct_dcm";
  dicomSvc->ExportPatientToCT(output_dir,dciom_dir);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Exports the current world geometry to a GDML file in the output directory.
 *
 * The GDML file is named using the configured world file name. Marks the GDML export as completed.
 */
void GeoSvc::WriteWorldToGdml(){
  
      DEBUG_GEO("Writing World Geometry To GDML...");
  World()->ExportToGDML(GetOutputDir(),m_world_file_name+".gdml");
  m_is_gdml_exported = true;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Compute and ensure existence of the geometry output directory used for exports.
 *
 * Constructs the directory path by joining RunSvc's `OutputDir` and GeoSvc's `GeoDir`, creates the directory if it does not exist, and returns the resulting path.
 *
 * @return std::string The full path to the geometry output directory.
 */
std::string GeoSvc::GetOutputDir() {
  auto dir = Service<ConfigSvc>()->GetValue<std::string>("RunSvc","OutputDir")+
            "/"+Service<ConfigSvc>()->GetValue<std::string>("GeoSvc","GeoDir");
  IO::CreateDirIfNotExits(dir);
  return dir;
}