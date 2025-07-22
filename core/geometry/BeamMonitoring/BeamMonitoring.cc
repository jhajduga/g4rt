#include "BeamMonitoring.hh"
#include "BeamMonitoringSD.hh"
#include "G4SystemOfUnits.hh"
#include "Services.hh"
#include "WorldConstruction.hh"
#include "G4SDManager.hh"
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs a BeamMonitoring instance and initializes its configuration.
 *
 * Calls the Configure() method to set up default configuration parameters for beam monitoring volumes.
 */
BeamMonitoring::BeamMonitoring(): IPhysicalVolume("BeamMonitoring"), Configurable("BeamMonitoring"){
  Configure();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Destructor for the BeamMonitoring class.
 *
 * Unregisters the configuration for this instance and releases resources associated with scoring plane physical volumes.
 */
BeamMonitoring::~BeamMonitoring() {
  configSvc()->Unregister(thisConfig()->GetName());
  Destroy();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Sets up configuration units and initializes default configuration for beam monitoring.
 *
 * Defines the "ScoringZPositions" configuration unit and applies default configuration values for all parameters. Prints the current configuration state.
 */
void BeamMonitoring::Configure() {
  // G4cout << "\n\n[INFO]::  Configuring the " << thisConfig()->GetName() << G4endl;
  
  DefineUnit<VecG4doubleSPtr>("ScoringZPositions");
  Configurable::DefaultConfig();   // setup the default configuration for all defined units/parameters
  Configurable::PrintConfig();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Sets default configuration values for the specified unit.
 *
 * For the "Label" unit, assigns the default label "Beam monitoring". For the "ScoringZPositions" unit, initializes a vector of doubles with default Z positions at -30 cm and -15 cm.
 *
 * @param unit The configuration unit to set default values for.
 */
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
/**
 * @brief Placeholder for writing beam monitoring information.
 *
 * Intended for future implementation to output or log relevant information about the beam monitoring setup.
 */
void BeamMonitoring::WriteInfo() {
  /// implement me.
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Destroys all scoring plane physical volumes managed by BeamMonitoring.
 *
 * Deletes each scoring plane physical volume and resets its pointer to nullptr to release resources.
 */
void BeamMonitoring::Destroy() {
  G4cout << "[INFO]:: \tDestroing the BeamMonitoring volume(s). " << G4endl;
  for(auto& plane : m_scoringPlanesPV){
    delete plane;
    plane = nullptr;
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs beam monitoring scoring planes within the parent world volume.
 *
 * Creates thin scoring plane volumes at configured Z positions, including an additional plane just above the patient environment. Each scoring plane is placed as a physical volume within the parent world and stored for later management. If the scoring positions configuration is unavailable, logs an error.
 *
 * @param parentWorld Pointer to the parent world physical volume where scoring planes will be placed.
 */
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
    auto patientEnvZPos = configSvc()->GetValue<double>("PatientGeometry", "PatientIsocentreZ");
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
/**
 * @brief Handles configuration updates for the BeamMonitoring instance.
 *
 * Checks if the configuration has been updated and logs updated parameters. Always returns true.
 *
 * @return G4bool Always returns true.
 */
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
/**
 * @brief Creates and registers the beam monitoring sensitive detector, assigning it to the scoring plane logical volume.
 *
 * If the sensitive detector does not already exist, it is instantiated and registered with the Geant4 sensitive detector manager. The detector is then assigned to the logical volume named "BeamScoringPlaneLV" in the world geometry.
 */
void BeamMonitoring::DefineSensitiveDetector(){

  if(m_beamMonitoringSD.Get()==0){
    auto beamSD = new BeamMonitoringSD("BeamSD");
    m_beamMonitoringSD.Put(beamSD);
  }
  G4SDManager::GetSDMpointer()->AddNewDetector(m_beamMonitoringSD.Get());
  Service<GeoSvc>()->World()->SetSensitiveDetector("BeamScoringPlaneLV", m_beamMonitoringSD.Get());
}

