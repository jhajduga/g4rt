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
 * @brief Returns the singleton instance of the LinacGeometry class.
 *
 * Ensures that only one instance of LinacGeometry exists throughout the application.
 *
 * @return Pointer to the global LinacGeometry instance.
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
 * @brief Destroys the linac gantry geometry and associated accelerator head volumes.
 *
 * Releases and deletes the current physical volume and, if present, destroys the accelerator head instance and resets its pointer.
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
 * @brief Constructs the linac world volume and places the accelerator head within the simulation.
 *
 * Creates the accelerator world as a box with configurable size and material, places it relative to the parent physical volume, and delegates construction of the accelerator head geometry to the selected head instance.
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
 * @brief Resets the accelerator head geometry to its initial state.
 *
 * If an accelerator head instance exists, calls its reset method to restore default configuration or state.
 */
void LinacGeometry::ResetHead() {
  if (m_headInstance) m_headInstance->Reset();
}



/**
 * @brief Sets the isocentre distance for the linac geometry.
 *
 * @param distance_cm The source-to-isocentre distance in centimeters.
 */
void LinacGeometry::SetIsocentreDistance(double distance_cm) {
  s_isocentre_distance_cm = distance_cm;
}

/**
 * @brief Returns the current isocentre distance in centimeters.
 *
 * @return The isocentre distance value in centimeters.
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