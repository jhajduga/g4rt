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
///
GeoSvc::GeoSvc() : TomlConfigurable("GeoSvc"), Logable("GeoAndScoring") {
  Configure();
}

////////////////////////////////////////////////////////////////////////////////
///
GeoSvc::~GeoSvc() {
  configSvc()->Unregister(thisConfig()->GetName());
}

////////////////////////////////////////////////////////////////////////////////
///
GeoSvc *GeoSvc::GetInstance() {
  static GeoSvc instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
///
bool GeoSvc::IsWorldBuilt() const { return my_world ? true : false; }

////////////////////////////////////////////////////////////////////////////////
///
void GeoSvc::Configure() {
  //G4cout << "[INFO]:: GeoSvc :: Service default configuration " << G4endl;

  DefineUnit<G4String>("HeadModel");
  DefineUnit<G4String>("MlcModel");
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
///
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
    m_config->SetValue(unit, G4String("Varian-HD120")); 
    // m_config->SetValue(unit, G4String("Simplified")); 
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
///
void GeoSvc::Initialize() {
  if (!m_isInitialized) {
    LOGSVC_INFO("Service initialization...");
    PrintConfig();

    if (m_configSvc->GetValue<bool>("RunSvc", "SavePhSp")) 
      ParseSavePhspPlaneRequest();

    GetHeadModel(); // verify the specified head model
    GetMlcModel();  // verify the specified MLC model

    m_isInitialized = true;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
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
///
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
///
void GeoSvc::Destroy() {
  G4cout << "[INFO]:: GeoSvc :: destroying geometry world" << G4endl;
  if (my_world) {
    my_world->Destroy();
    my_world = nullptr;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
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
///
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
///
EMlcModel GeoSvc::GetMlcModel() const {
  auto mlcName = m_configSvc->GetValue<G4String>("GeoSvc", "MlcModel");
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
///
VPatient* GeoSvc::Patient(){
  if(World()->PatientEnvironment())
    return World()->PatientEnvironment()->GetPatient();
  return nullptr;
}

////////////////////////////////////////////////////////////////////////////////
///
std::vector<VPatient*> GeoSvc::CustomDetectors(){
  return World()->GetCustomDetectors();
}

////////////////////////////////////////////////////////////////////////////////
/// TODO: use enum types!!!
VMlc* GeoSvc::MLC(){
  if(thisConfig()->GetValue<G4String>("HeadModel")){
    return BeamCollimation::GetInstance()->GetMlc();
  } 
  return nullptr; // this should never happen, but prevent warning
}


////////////////////////////////////////////////////////////////////////////////
///
void GeoSvc::RegisterScoringComponent(const GeoComponet *element){
  m_scoring_components.emplace_back(element);
}

////////////////////////////////////////////////////////////////////////////////
///
void GeoSvc::PerformDefaultExport() {
  WriteWorldToTFile();
}

////////////////////////////////////////////////////////////////////////////////
///
void GeoSvc::PerformRequestedExport() {
  if(thisConfig()->GetValue<bool>("ExportGeometryToTFile") && !m_is_tfile_exported)
    WriteWorldToTFile();
  if(thisConfig()->GetValue<bool>("ExportScoringGeometryToCsv"))
    WriteScoringComponentsPositioningToCsv();
  if(thisConfig()->GetValue<bool>("ExportGeometryToGate"))
    ExportToGateGenericRepeater();
  if(thisConfig()->GetValue<bool>("ExportGeometryToGDML"))
    WriteWorldToGdml();
  auto patient = Service<ConfigSvc>()->GetValue<std::string>("PatientGeometry","Type");
  if(patient.find("D3DDetector")!=std::string::npos)
    ExportDose3DLayerPads();
}


////////////////////////////////////////////////////////////////////////////////
///
void GeoSvc::ExportToGateGenericRepeater() const {
  std::cout << "[INFO] GeoSvc:: ExportToGateGenericRepeater..." <<std::endl;
  std::string output_dir = Service<ConfigSvc>()->GetValue<std::string>("RunSvc","OutputDir");
  for(const auto& gc : m_scoring_components){
      // GATE like export
      gc->ExportToGateCsv(output_dir);
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void GeoSvc::WriteScoringComponentsPositioningToCsv() const {
  LOGSVC_INFO("Writing Scroing Components to CSV...");
  std::string output_dir = GetOutputDir();
  for(const auto& gc : m_scoring_components){
      // Generic geometry export
      gc->ExportCellPositioningToCsv(output_dir);
      // Voxelised geometry export
      gc->ExportVoxelPositioningToCsv(output_dir);
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void GeoSvc::WriteScoringComponentsPositioningToTFile() const {
  LOGSVC_INFO("Writing Scroing Components to TFile...");
  std::string output_dir = GetOutputDir();
  for(const auto& gc : m_scoring_components){
      LOGSVC_INFO("Implement me ...");
      // gc->ExportPositioningToTFile(output_dir); to be repaired to new scoring maps scheme
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void GeoSvc::ExportDose3DLayerPads() const{
  // TODO!!!: Remove me in future!
  std::cout << "[INFO] GeoSvc:: ExportDose3DLayerPads..." <<std::endl;
  std::string output_dir = Service<ConfigSvc>()->GetValue<std::string>("RunSvc","OutputDir");
  for(const auto& gc : m_scoring_components){
    if(auto d3d = dynamic_cast<const D3DDetector*>(gc))
      d3d->ExportLayerPads(output_dir);
  }
}

////////////////////////////////////////////////////////////////////////////////
/// Write geometry as TGeometry object in TFile
void GeoSvc::WriteWorldToTFile() {
  LOGSVC_DEBUG("Writing World Geometry To TFile...");
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
  setNodesVisByMaterial("PMMA",38,50);

  // Dose3D visibility 
  setNodesVisByName("D3D",49);
  setNodesVisByName("Stl",12,50);

  // Final export
  geo_dir->WriteTObject(tgeom,"World_Geometry");
  tgeom->UnlockGeometry();
  LOGSVC_INFO("Writing to {} - done!",geo_tfile);
  m_is_tfile_exported = true;
}

////////////////////////////////////////////////////////////////////////////////
///
void GeoSvc::WritePatientToCsvCT(){
  auto output_dir = GetOutputDir()+"/dicom/ct_csv";
  PatientGeometry::GetInstance()->ExportToCsvCT(output_dir);
}

////////////////////////////////////////////////////////////////////////////////
///
void GeoSvc::WritePatientToDicomCT(){
  // NOTE: Currently this service is using the csv data, 
  //       hence the GeoSvc::WritePatientToCsvCT has to be called!
  auto output_dir = GetOutputDir()+"/dicom/ct_csv";
  auto dicomSvc = Service<DicomSvc>();
  auto dciom_dir = GetOutputDir()+"/dicom/ct_dcm";
  dicomSvc->ExportPatientToCT(output_dir,dciom_dir);
}

////////////////////////////////////////////////////////////////////////////////
///
void GeoSvc::WriteWorldToGdml(){
  LOGSVC_DEBUG("Writing World Geometry To GDML...");
  World()->ExportToGDML(GetOutputDir(),m_world_file_name+".gdml");
  m_is_gdml_exported = true;
}

////////////////////////////////////////////////////////////////////////////////
///
std::string GeoSvc::GetOutputDir() {
  auto dir = Service<ConfigSvc>()->GetValue<std::string>("RunSvc","OutputDir")+
            "/"+Service<ConfigSvc>()->GetValue<std::string>("GeoSvc","GeoDir");
  IO::CreateDirIfNotExits(dir);
  return dir;
}