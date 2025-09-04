#include "SavePhSp.hh"
#include "G4SystemOfUnits.hh"
#include "Services.hh"

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Create a SavePhSp instance and initialize its configuration.
 *
 * Constructs the SavePhSp physical-volume manager, initializing its IPhysicalVolume
 * and Configurable bases with the name "SavePhSp" and immediately invoking
 * Configure() to apply default and registered configuration settings.
 */
SavePhSp::SavePhSp(): IPhysicalVolume("SavePhSp"), Configurable("SavePhSp"){
  Configure();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Destroy the SavePhSp instance, unregistering its configuration and freeing resources.
 *
 * Unregisters this object's configuration from the configuration service and destroys any
 * phase-space physical volumes created and tracked by this instance to prevent resource leaks.
 */
SavePhSp::~SavePhSp() {
  configSvc()->Unregister(thisConfig()->GetName());
  Destroy();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Initializes and prints the default configuration for the SavePhSp volume.
 *
 * Sets up default configuration parameters for all defined units and outputs the current configuration state.
 */
void SavePhSp::Configure() {
  G4cout << "\n\n[INFO]::  Configuring the " << thisConfig()->GetName() << G4endl;

  Configurable::DefaultConfig();   // setup the default configuration for all defined units/parameters
  Configurable::PrintConfig();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Sets the default configuration value for the specified unit.
 *
 * If the unit is "Label", assigns the default label "Save phase space" to the configuration.
 *
 * @param unit The configuration unit name to set a default value for.
 */
void SavePhSp::DefaultConfig(const std::string &unit) {

  // Volume name
  if (unit.compare("Label") == 0)
    thisConfig()->SetValue(unit, std::string("Save phase space"));
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Write information about the saved phase-space volumes.
 *
 * Reserved placeholder for emitting human- or machine-readable metadata about
 * the configured/constructed phase-space volumes managed by SavePhSp.
 *
 * Currently unimplemented and performs no action.
 */
void SavePhSp::WriteInfo() {
  /// implement me.
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Release and delete all user-defined phase-space placements.
 *
 * Deletes each G4PVPlacement pointer stored in m_usrPhspPV, freeing their memory.
 * After this call the pointers in m_usrPhspPV are deleted (the container itself is not modified),
 * so callers must not dereference or use those pointers afterwards.
 * An informational message is emitted when destruction starts.
 */
void SavePhSp::Destroy() {
  G4cout << "[INFO]:: \tDestroing the SavePhSp volume. " << G4endl;
  for(auto& phsp : m_usrPhspPV){
    delete phsp;
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs and places phase space volumes at user-defined Z positions within the parent world volume.
 *
 * Creates thin box-shaped phase space regions matching the XY dimensions of the parent world and places them at specified Z coordinates. Each region is filled with air and uniquely named based on its position. The created physical volumes are stored for later management.
 *
 * @param parentWorld The parent Geant4 physical volume in which the phase space regions are placed.
 */
void SavePhSp::Construct(G4VPhysicalVolume *parentWorld) {
  G4cout << "[INFO]:: SavePhSp construction... " << G4endl;
  auto Air = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "Usr_G4AIR20C");
  auto worldBox = dynamic_cast<G4Box*>(parentWorld->GetLogicalVolume()->GetSolid());
  auto halfSizeX = worldBox->GetXHalfLength();
  auto halfSizeY = worldBox->GetYHalfLength();
  auto phspBox = new G4Box("PhSpBox", halfSizeX, halfSizeY, 1 * um);
  auto phspBoxLV = new G4LogicalVolume(phspBox, Air.get(), "PhSpBoxLV", 0, 0, 0);
  // define the placement of the phsp
  G4String boxName;
  for (auto iPhspZPosition : *(configSvc()->GetValue<std::vector<G4double> *>("GeoSvc", "SavePhSpUsr"))) {
    G4cout << "[INFO]:: WorldConstruction:: adding phsp at :" << abs(iPhspZPosition) / cm << "[cm] above isocentre"<< G4endl;
    boxName = "PhSpBox" + svc::to_string(iPhspZPosition);
    // phase space volume should be centered around the user requested plane (to set correctly preStepPoint)
    m_usrPhspPV.push_back(new G4PVPlacement(0, G4ThreeVector(0. * cm, 0. * cm, iPhspZPosition), boxName, phspBoxLV, parentWorld, false, 0));
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Handle configuration changes and report updated configuration units.
 *
 * If the configuration is active, iterates the configuration unit names and reports which unit-level parameters are active.
 * The function performs only status reporting and always returns true.
 *
 * @return G4bool Always returns true.
 */
G4bool SavePhSp::Update() {

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


