#include "BeamMonitoring.hh"
#include "BeamMonitoringSD.hh"
#include "G4SystemOfUnits.hh"
#include "Services.hh"
#include "WorldConstruction.hh"
#include "G4SDManager.hh"
////////////////////////////////////////////////////////////////////////////////
///
BeamMonitoring::BeamMonitoring(): IPhysicalVolume("BeamMonitoring"), Configurable("BeamMonitoring"){
  Configure();
}

////////////////////////////////////////////////////////////////////////////////
///
BeamMonitoring::~BeamMonitoring() {
  configSvc()->Unregister(thisConfig()->GetName());
  Destroy();
}

////////////////////////////////////////////////////////////////////////////////
///
void BeamMonitoring::Configure() {
  // G4cout << "\n\n[INFO]::  Configuring the " << thisConfig()->GetName() << G4endl;
  
  DefineUnit<VecG4doubleSPtr>("ScoringZPositions");
  Configurable::DefaultConfig();   // setup the default configuration for all defined units/parameters
  Configurable::PrintConfig();
}

////////////////////////////////////////////////////////////////////////////////
///
void BeamMonitoring::DefaultConfig(const std::string &unit) {

  // Volume name
  if (unit.compare("Label") == 0)
    thisConfig()->SetValue(unit, std::string("Beam monitoring"));
  
  if (unit.compare("ScoringZPositions") == 0) {
    thisConfig()->SetValue(unit, std::make_shared<VecG4double>());
    auto vecPtr = thisConfig()->GetValue<VecG4doubleSPtr>(unit);
    vecPtr->emplace_back(-30.* cm); // PRIMO PhSp s2 position: 30.025 cm
    vecPtr->emplace_back(-15* cm);
    // G4cout << "[DEBUG]:: GeoSvc::DefaultConfig value seted:  " << unit << G4endl;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void BeamMonitoring::WriteInfo() {
  /// implement me.
}

////////////////////////////////////////////////////////////////////////////////
///
void BeamMonitoring::Destroy() {
  G4cout << "[INFO]:: \tDestroing the BeamMonitoring volume(s). " << G4endl;
  for(auto& plane : m_scoringPlanesPV){
    delete plane;
    plane = nullptr;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void BeamMonitoring::Construct(G4VPhysicalVolume *parentWorld) {
  G4cout << "[INFO]:: BeamMonitoring construction... " << G4endl;
  const VecG4doubleWPtr& scoringZPositionWPtr = thisConfig()->GetValue<VecG4doubleSPtr>("ScoringZPositions");
  if(!scoringZPositionWPtr.expired()) {
    auto scoringZPositionSPtr = scoringZPositionWPtr.lock();
    auto Air = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "Usr_G4AIR20C");
    auto worldBox = dynamic_cast<G4Box*>(parentWorld->GetLogicalVolume()->GetSolid());
    auto halfSizeX = worldBox->GetXHalfLength();
    auto halfSizeY = worldBox->GetYHalfLength();
    auto scoringPlane = new G4Box("BeamScoringPlane", halfSizeX, halfSizeY, 1 * um);
    auto scoringPlaneLV = new G4LogicalVolume(scoringPlane, Air.get(), "BeamScoringPlaneLV", 0, 0, 0);
    G4String name;
    G4int id = 0;
    // add scoring position just above the PatientEnvZPosition
    auto patientEnvZSize = configSvc()->GetValue<double>("PatientGeometry", "EnviromentSizeZ");
    auto patientEnvZPos = configSvc()->GetValue<double>("PatientGeometry", "EnviromentPositionZ");
    scoringZPositionSPtr->push_back( patientEnvZPos - patientEnvZSize/2. - 1);
    for (const auto &iPlaneZPosition : *scoringZPositionSPtr) {
      G4cout << "[INFO]:: WorldConstruction:: adding  beam monitoring plane at :"
             << abs(iPlaneZPosition) / cm << "[cm] above iso-centre" << G4endl;
      name = "beamScoringPlane_" + svc::to_string(id++);
      // monitoring plane volume should be centered around the user requested plane (to set correctly preStepPoint)
      m_scoringPlanesPV.push_back(
          new G4PVPlacement(0, G4ThreeVector(0., 0., iPlaneZPosition), name, scoringPlaneLV, parentWorld, false, 0));
    }
  } else {
    G4cout << "[ERROR]:: BeamMonitoring construction:: \"ScoringZPositions\" config expired! " << G4endl;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
G4bool BeamMonitoring::Update() {

  if (thisConfig()->GetStatus()) {
    G4cout << "[INFO]:: " << thisConfig()->GetName() << " configuration has been updated..." << G4endl;
    for (auto param : thisConfig()->GetUnitsNames()) {
      if (thisConfig()->GetStatus(param)) {
        G4cout << "[INFO]:: Updated:: " << param << " : value will be printed here (implement me)." << G4endl;
      }
    }
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
void BeamMonitoring::DefineSensitiveDetector(){

  if(m_beamMonitoringSD.Get()==0){
    auto beamSD = new BeamMonitoringSD("BeamSD");
    m_beamMonitoringSD.Put(beamSD);
  }
  G4SDManager::GetSDMpointer()->AddNewDetector(m_beamMonitoringSD.Get());
  Service<GeoSvc>()->World()->SetSensitiveDetector("BeamScoringPlaneLV", m_beamMonitoringSD.Get());
}

