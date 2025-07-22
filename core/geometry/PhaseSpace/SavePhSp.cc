#include "SavePhSp.hh"
#include "G4SystemOfUnits.hh"
#include "Services.hh"

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs a SavePhSp object and initializes its configuration.
 *
 * Initializes the SavePhSp phase space volume by invoking the configuration setup upon creation.
 */
SavePhSp::SavePhSp(): IPhysicalVolume("SavePhSp"), Configurable("SavePhSp"){
  Configure();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Destructor for the SavePhSp class.
 *
 * Unregisters the configuration associated with this instance and releases all allocated phase space physical volumes.
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
 * @brief Placeholder for writing phase space volume information.
 *
 * This method is not yet implemented.
 */
void SavePhSp::WriteInfo() {
  /// implement me.
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Deletes all dynamically allocated phase space physical volumes managed by this instance.
 *
 * Frees memory for each user-defined phase space physical volume stored in the internal vector.
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
 * @brief Updates the configuration status and reports any changes.
 *
 * Checks if the configuration is active and, if so, outputs informational messages for each updated configuration parameter. Always returns true.
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


