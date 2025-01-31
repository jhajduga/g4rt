#include "WaterPhantom.hh"
#include "G4SystemOfUnits.hh"
#include "G4Box.hh"
#include "G4ProductionCuts.hh"
#include "WaterPhantomSD.hh"
#include "NTupleEventAnalisys.hh"
#include "G4UserLimits.hh"
#include "toml.hh"
#include "Services.hh"

////////////////////////////////////////////////////////////////////////////////
///
WaterPhantom::WaterPhantom():VPatient("WaterPhantom"){}

////////////////////////////////////////////////////////////////////////////////
///
WaterPhantom::~WaterPhantom() {
  Destroy();
}

////////////////////////////////////////////////////////////////////////////////
///
void WaterPhantom::ParseTomlConfig(){
  auto configFile = GetTomlConfigFile();
  auto configPrefix = GetTomlConfigPrefix();
  LOGSVC_INFO("Importing configuration from:\n{}",configFile);

  if (!svc::checkIfFileExist(configFile)) {
    LOGSVC_CRITICAL("File {} not fount.", configFile);
    G4Exception("WaterPhantom", "ParseTomlConfig", FatalErrorInArgument, "");
  }

  std::string configObjDetector("Detector");
  std::string configObjScoring("Scoring");
  if(!configPrefix.empty()){ // here it's assummed that the config data is given with prefixed name
    configObjDetector.insert(0,configPrefix+"_");
    configObjScoring.insert(0,configPrefix+"_");
  }
  else {
    G4String msg = "The configuration PREFIX is not defined";
    LOGSVC_CRITICAL(msg.data());
    G4Exception("WaterPhantom", "ParseTomlConfig", FatalErrorInArgument, msg);
  }
  
  auto config = toml::parse_file(configFile);

  m_centrePositionX = config[configObjDetector]["TranslationFromCentre"][0].value_or(0.0);
  m_centrePositionY = config[configObjDetector]["TranslationFromCentre"][1].value_or(0.0);
  m_centrePositionZ = config[configObjDetector]["TranslationFromCentre"][2].value_or(0.0);
  m_detectorVoxelizationX = config[configObjDetector]["Voxelization"][0].value_or(0);
  m_detectorVoxelizationY = config[configObjDetector]["Voxelization"][1].value_or(0);
  m_detectorVoxelizationZ = config[configObjDetector]["Voxelization"][2].value_or(0);
  ///
  m_sizeX = config[configObjDetector]["Size"][0].value_or(0.0);
  m_sizeY = config[configObjDetector]["Size"][1].value_or(0.0);
  m_sizeZ = config[configObjDetector]["Size"][2].value_or(0.0);
  /// 
  detectorMediumName = config[configObjDetector]["Medium"].value_or("");

  auto env_pos_x = Service<ConfigSvc>()->GetValue<double>("PatientGeometry", "EnviromentPositionX");
  auto env_pos_y = Service<ConfigSvc>()->GetValue<double>("PatientGeometry", "EnviromentPositionY");
  auto env_pos_z = Service<ConfigSvc>()->GetValue<double>("PatientGeometry", "EnviromentPositionZ");

  auto top_position_in_env = G4ThreeVector(m_centrePositionX,m_centrePositionY,m_centrePositionZ + m_sizeZ);
  m_patient_top_position_in_world_env = G4ThreeVector(env_pos_x,env_pos_y,env_pos_z) + top_position_in_env;

  
  ///
  m_watertankScoring = config[configObjScoring]["FullVolume"].value_or(true);
  m_farmerScoring = config[configObjScoring]["FarmerDoseCalibration"].value_or(false);

  ///
  m_tracks_analysis = config[configObjScoring]["TracksAnalysis"].value_or(false);

}

////////////////////////////////////////////////////////////////////////////////
///
G4bool WaterPhantom::LoadDefaultParameterization(){
  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
void WaterPhantom::WriteInfo() {
  auto configSvc = Service<ConfigSvc>();
  G4String info;
  info = "Size of the water phantom: "
       + std::to_string (m_sizeX / cm) + " x "
       + std::to_string (m_sizeY / cm) + " x "
       + std::to_string (m_sizeZ / cm) + " [cm^3]";
  G4ThreeVector translation(configSvc->GetValue<double>("PatientGeometry","EnviromentPositionX"),
                            configSvc->GetValue<double>("PatientGeometry","EnviromentPositionY"),
                            configSvc->GetValue<double>("PatientGeometry","EnviromentPositionZ"));
  LOGSVC_INFO(info);                            
  info = "Centre of the water phantom environment: (" 
         + std::to_string( translation.getX() / cm) 
         + "," + std::to_string( translation.getX() / cm) 
         + "," + std::to_string( translation.getX() / cm) + ") [cm]\n";
  LOGSVC_INFO(info);                            

  G4ThreeVector centre(m_centrePositionX*mm , m_centrePositionY*mm  , m_centrePositionZ*mm);
  LOGSVC_INFO("Centre of the water phantom (within the phantom world environment): {} [cm]\n", centre / cm);
}

////////////////////////////////////////////////////////////////////////////////
///
void WaterPhantom::Destroy() {
  LOGSVC_INFO("Destroing the WaterPhantom volume.");
  auto phantomVolume = GetPhysicalVolume();
  if (phantomVolume) {
    delete phantomVolume;
    SetPhysicalVolume(nullptr);
  }
}

////////////////////////////////////////////////////////////////////////////////
///
G4bool WaterPhantom::LoadParameterization(){
  // Configurable::ValidateConfig();
  if(IsTomlConfigExists()){
    ParseTomlConfig();
  }
  else{
    LoadDefaultParameterization();
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
void WaterPhantom::Construct(G4VPhysicalVolume *parentWorld) {
  
  LoadParameterization();
  
  auto medium = Service<ConfigSvc>()->GetValue<G4MaterialSPtr>("MaterialsSvc", detectorMediumName);

  // create a phantom box filled with water, with given side dimensions
  auto waterPhantomBox = new G4Box("waterPhantomBox", m_sizeX / 2., m_sizeY / 2., m_sizeZ / 2.);

  auto waterPhantomLV = new G4LogicalVolume(waterPhantomBox, medium.get(), "waterPhantomLV");

  // the placement of phantom center in the gantry (global) coordinate system that is managed by PatientGeometry class
  // here we locate the phantom box in the center of envelope box created in PatientGeometry:
  SetPhysicalVolume(new G4PVPlacement(nullptr, G4ThreeVector(m_centrePositionX*mm , m_centrePositionY*mm  , m_centrePositionZ*mm), "WaterPhantomPV", waterPhantomLV, parentWorld, false, 0));

  // Region for cuts
  auto regVol = new G4Region("waterPhantomR");
  auto cuts = new G4ProductionCuts;
  cuts->SetProductionCut(0.1 * mm);
  regVol->SetProductionCuts(cuts);
  waterPhantomLV->SetRegion(regVol);
  regVol->AddRootLogicalVolume(waterPhantomLV);


}


////////////////////////////////////////////////////////////////////////////////
///
G4bool WaterPhantom::Update() {
  // TODO implement me.
  return true;
}


////////////////////////////////////////////////////////////////////////////////
///
void WaterPhantom::ConstructSensitiveDetector(){
  if(m_patientSD.Get()==0){
    // TODO: To be veryfied
    auto centre = GetPhysicalVolume()->GetTranslation();
    centre += GetParentPtr()->GetPhysicalVolume()->GetTranslation();
    m_patientSD.Put(new WaterPhantomSD("PhantomSD",centre));
    m_patientSD.Get()->SetTracksAnalysis(m_tracks_analysis);
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void WaterPhantom::ConstructFullVolumeScoring(const G4String& name){
  if(m_patientSD.Get()==0)
    ConstructSensitiveDetector();
  auto patientSD = m_patientSD.Get();
  auto envBox = dynamic_cast<G4Box*>(GetPhysicalVolume()->GetLogicalVolume()->GetSolid());
  patientSD->AddScoringVolume(name,name,*envBox,m_detectorVoxelizationX,
                                                m_detectorVoxelizationY,
                                                m_detectorVoxelizationZ);
}

////////////////////////////////////////////////////////////////////////////////
///
void WaterPhantom::ConstructFarmerVolumeScoring(const G4String& name){
  if(m_patientSD.Get()==0)
    ConstructSensitiveDetector();
  auto patientSD = m_patientSD.Get();
  auto farmerBox = G4Box(name+"Box",2.65*mm,2.65*mm,20*cm); 
  patientSD->AddScoringVolume(name,name,farmerBox,1,1,200);
}
////////////////////////////////////////////////////////////////////////////////
///
void WaterPhantom::DefineSensitiveDetector(){
  if(m_patientSD.Get()==0){
    if(m_watertankScoring){
      ConstructFullVolumeScoring("WaterTank");
    }
    if(m_farmerScoring){
      ConstructFarmerVolumeScoring("Farmer30013");
    }
    //
    VPatient::SetSensitiveDetector("waterPhantomLV", m_patientSD.Get());
  }
}


