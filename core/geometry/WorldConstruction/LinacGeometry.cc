#include "LinacGeometry.hh"
#include "BeamCollimation.hh"
#include "Services.hh"
#include "G4SystemOfUnits.hh"
#include "G4GeometryManager.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "G4RunManager.hh"
#include "G4Box.hh"
#include "G4Run.hh"

namespace {
  G4Mutex headConstructionMutex = G4MUTEX_INITIALIZER;
}


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs a LinacGeometry instance and initializes its configuration.
 *
 * Initializes the LinacGeometry object by setting up default configuration parameters.
 */
LinacGeometry::LinacGeometry() 
: Configurable("LinacGeometry"), IPhysicalVolume("LinacGeometry") {
  Configure();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Get the single global LinacGeometry instance.
 *
 * @return LinacGeometry* Pointer to the single global LinacGeometry instance.
 */
LinacGeometry *LinacGeometry::GetInstance() {
  static LinacGeometry instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Sets up the default configuration parameters for the LinacGeometry.
 *
 * Defines configuration units for the linac label, envelope box size, and source-to-isocentre distance (SID), then applies and prints the default configuration values.
 */
void LinacGeometry::Configure() {
  G4cout << "\n\n[INFO]::  Default configuration of the " << thisConfig()->GetName() << G4endl;

  DefineUnit<std::string>("Label");
  DefineUnit<G4ThreeVector>("LinacEnvelopeBoxSize");
  DefineUnit<G4double>("SID"); // Source to Isocentre Distance

  Configurable::DefaultConfig();   // setup the default configuration for all defined units/parameters
  Configurable::PrintConfig();

}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Sets the default configuration value for a specified unit.
 *
 * Assigns a default value to the given configuration unit, such as the label, envelope box size, or source-to-isocentre distance, if it matches a recognized unit name.
 *
 * @param unit The name of the configuration unit to set a default value for.
 */
void LinacGeometry::DefaultConfig(const std::string &unit) {

  // Volume name
  if (unit.compare("Label") == 0)
    thisConfig()->SetValue(unit, std::string("Linac Construction Environment"));

  if (unit.compare("LinacEnvelopeBoxSize") == 0)
    thisConfig()->SetValue(unit, G4ThreeVector(2000., 2000., 1500.)); // [mm]

  if (unit.compare("SID") == 0)
    thisConfig()->SetValue(unit, G4double(1000.)); // [mm]
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Selects and instantiates the appropriate accelerator head model for the linac geometry.
 *
 * Determines the head model type from the geometry service configuration, creates the corresponding head instance, and stores it internally. Returns true if a head accelerator instance was successfully created.
 *
 * @return true if a head accelerator instance exists after selection; false otherwise.
 */
bool LinacGeometry::design() {
  auto geoSvc = Service<GeoSvc>();
  G4cout << "I'm building " << geoSvc->thisConfig()->GetValue<G4String>("HeadModel") << "  geometry..." << G4endl;
  bool bAccExists = false;
  // switch between different types:
  switch (geoSvc->GetHeadModel()) {
    case EHeadModel::BeamCollimation:
      m_headInstance = BeamCollimation::GetInstance();
      bAccExists = true;
      break;
  }
  return bAccExists;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Destroy the Linac geometry and associated accelerator head.
 *
 * Destroys the current accelerator head instance (if any), deletes the stored
 * physical volume representing the gantry/world, and clears internal pointers
 * so the geometry can be safely reconstructed or the object cleaned up.
 *
 * Side effects:
 * - Calls `Destroy()` on the head instance and sets `m_headInstance` to nullptr.
 * - Deletes the physical volume returned by `GetPhysicalVolume()` and calls
 *   `SetPhysicalVolume(nullptr)`.
 */
void LinacGeometry::Destroy() {
  G4cout << "[INFO]:: \t\tDestroing the Gantry volumes... " << G4endl;
  auto headPV = GetPhysicalVolume();
  if (headPV) {
    if (m_headInstance){ 
      m_headInstance->Destroy();
      m_headInstance = nullptr;
    }
    delete headPV;
    SetPhysicalVolume(nullptr);
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Updates the accelerator head geometry if it exists.
 *
 * @return `true` if the update succeeds or no head instance is present; `false` if the head instance update fails.
 */
G4bool LinacGeometry::Update() {
  if (m_headInstance) {
    if (!m_headInstance->Update()) return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Construct the linac world volume and instantiate the selected accelerator head.
 *
 * Constructs a box-shaped world volume sized using the "LinacEnvelopeBoxSize" configuration value and made from the configured material (defaults to "G4_Galactic"), places that world under the provided parent physical volume, and delegates construction of the accelerator head to the selected head instance.
 *
 * The created world physical volume is placed with a fixed translation of (0, 0, -1000). This method sets the LinacGeometry physical volume and relies on design() to select and create m_headInstance before delegating its construction.
 *
 * @param parentPV The parent physical volume to which the linac world volume is attached.
 */
void LinacGeometry::Construct(G4VPhysicalVolume *parentPV) {

  // a call to select the right accelerator
  design();
  // create the accelerator-world box
  // auto medium = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "Usr_G4AIR20C");
  auto medium = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_Galactic");
  auto envBoxSize = thisConfig()->GetValue<G4ThreeVector>("LinacEnvelopeBoxSize");
  auto accWorldB = new G4Box("accWorldG", envBoxSize.getX() / 2., envBoxSize.getY() / 2., envBoxSize.getZ() / 2.);
  auto accWorldLV = new G4LogicalVolume(accWorldB, medium.get(), "linacWorldLV", 0, 0, 0);

  // The centre of this PV is always 0,0,0, hence we need to only do the isoToSim translation
  SetPhysicalVolume(new G4PVPlacement(0, G4ThreeVector(0,0,-1000), "acceleratorBox", accWorldLV, parentPV, false, 0));

  // create the actual accelerator
  m_headInstance->IPhysicalVolume::Construct(this);
  // m_headInstance->WriteInfo();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Outputs the current accelerator rotation angle in degrees.
 *
 * Retrieves the rotation angle around the X-axis from the world construction configuration and prints it to the console.
 */
void LinacGeometry::WriteInfo() {
  G4double rotationX = configSvc()->GetValue<G4double>("WorldConstruction", "rotationX");
  G4cout << "Accelerator angle: " << rotationX / deg << " [deg]" << G4endl;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Restore the accelerator head to its initial configuration.
 *
 * If a head instance is present, its configuration and state are restored to defaults; otherwise no action is taken.
 */
void LinacGeometry::ResetHead() {
  if (m_headInstance) m_headInstance->Reset();
}



/**
 * @brief Set the global source-to-isocentre distance used by the linac geometry.
 *
 * Updates the static isocentre distance (in centimeters) read by geometry construction and positioning.
 *
 * @param distance_cm Source-to-isocentre distance in centimeters.
 * @see LinacGeometry::GetIsocentreDistance
 */
void LinacGeometry::SetIsocentreDistance(double distance_cm) {
  s_isocentre_distance_cm = distance_cm;
}

/**
 * @brief Retrieve the current isocentre distance used for geometry construction.
 *
 * @return The isocentre distance in centimeters.
 */
double LinacGeometry::GetIsocentreDistance() {
  return s_isocentre_distance_cm;
}

// ////////////////////////////////////////////////////////////////////////////////
// ///
// G4RotationMatrix *LinacGeometry::rotateHead() {
//   G4double rotationX = configSvc()->GetValue<G4double>("WorldConstruction", "rotationX");
//   G4RotationMatrix *rmInv = new G4RotationMatrix();
//   rmInv = rotateHead(rotationX);
//   return rmInv == nullptr ? rmInv : nullptr;
// }

// ////////////////////////////////////////////////////////////////////////////////
// /// TODO: refactor code to smart ptrs!
// G4RotationMatrix* LinacGeometry::rotateHead(G4double angleX) {
//   G4GeometryManager::GetInstance()->OpenGeometry();
//   G4ThreeVector NewCentre;
//   const G4RotationMatrixWPtr& rmWPtr = configSvc()->GetValue<G4RotationMatrixSPtr>("GeoSvc", "RotationMatrix");
//   if(!rmWPtr.expired()) {
//     auto rm = rmWPtr.lock();
//     auto headPV = GetPhysicalVolume();
//     auto rmInv = new G4RotationMatrix();
//     headPV->SetTranslation(G4ThreeVector());
//     headPV->SetRotation(rm.get());
//     if (configSvc()->GetValue<G4bool>("WorldConstruction", "Rotate90Y")) {
//       rm->rotateY(90. * deg);
//     }
//     rm->rotateX(-angleX);
//     headPV->SetRotation(rm.get());
//     *rmInv = CLHEP::inverseOf(*rm);
//     NewCentre = *rmInv * G4ThreeVector();
//     headPV->SetTranslation(NewCentre);
//     G4GeometryManager::GetInstance()->CloseGeometry();
//     G4RunManager::GetRunManager()->GeometryHasBeenModified();
//     return rmInv;
//   }
//   return nullptr;
// }

////////////////////////////////////////////////////////////////////////////////
/// NOTE: This method is called from WorldConstruction::ConstructSDandField
/**
 * @brief Defines sensitive detectors for the linac head geometry in a thread-safe manner.
 *
 * Acquires a mutex lock to ensure thread safety when defining sensitive detectors, particularly in multi-threaded execution. Delegates the sensitive detector definition to the current accelerator head instance if it exists.
 */
void LinacGeometry::DefineSensitiveDetector() {
  G4AutoLock lock(&headConstructionMutex);

  if(m_headInstance)
    m_headInstance->DefineSensitiveDetector();
}