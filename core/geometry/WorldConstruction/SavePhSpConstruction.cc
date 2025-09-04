#include "SavePhSpConstruction.hh"
#include "WaterPhantom.hh"
#include "WorldConstruction.hh"
#include "Services.hh"
#include "SavePhSp.hh"
#include "SavePhSpSD.hh"
#include "G4GeometryManager.hh"
#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "G4LogicalVolume.hh"
#include "G4VSensitiveDetector.hh"

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Obtain the singleton SavePhSpConstruction instance.
 *
 * Returns a pointer to the single, lazily-constructed SavePhSpConstruction
 * instance used application-wide. The instance is created on first call and
 * lives until program termination. Construction of the local static is thread-safe
 * (since C++11).
 *
 * @return SavePhSpConstruction* Pointer to the singleton instance.
 */
SavePhSpConstruction* SavePhSpConstruction::GetInstance() {
  static SavePhSpConstruction instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Initializes the phase space model by creating a new SavePhSp instance.
 *
 * @return true Always returns true after initialization.
 */
bool SavePhSpConstruction::ModelSetup() {
  //auto configSvc = Service<ConfigSvc>();
  // NOTE: Currently there is only one model of SavePhSp planes
  m_savePhSp = new SavePhSp();
  return true;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Releases resources associated with the phase space construction.
 *
 * Destroys the internal phase space model if it exists, deletes the world physical volume, and resets its pointer to nullptr.
 */
void SavePhSpConstruction::Destroy() {
  if (PVPhmWorld) {
    if (m_savePhSp) m_savePhSp->Destroy();
    delete PVPhmWorld;
    PVPhmWorld = nullptr;
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs phase space planes within the provided parent physical volume.
 *
 * Initializes the phase space model and delegates the construction of phase space planes to the internal `SavePhSp` instance, placing them directly in the main world volume without creating an enclosing box.
 *
 * @param parentPV The parent physical volume in which the phase space planes will be constructed.
 */
void SavePhSpConstruction::Construct(G4VPhysicalVolume *parentPV) {
  // A call to select the right model
  if (ModelSetup()) {
    // NOTE: Do not create the envelop box - the phase space planes are being created
    //       in the main world box:
    m_savePhSp->Construct(parentPV);
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Updates the phase space model state.
 *
 * Calls the `Update()` method on the internal `SavePhSp` instance if it exists. Returns `false` if the update fails; otherwise, returns `true`.
 *
 * @return `true` if the update succeeds or no model is present; `false` if the update fails.
 */
G4bool SavePhSpConstruction::Update() {
  if (m_savePhSp) {
    if (!m_savePhSp->Update()) return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Instructs the internal phase-space model to write its metadata/output.
 *
 * Calls SavePhSp::WriteInfo() if the internal SavePhSp instance has been created;
 * otherwise this is a no-op.
 */
void SavePhSpConstruction::WriteInfo() {
  if(m_savePhSp) m_savePhSp->WriteInfo();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Defines sensitive detectors for phase space planes in the simulation geometry.
 *
 * Retrieves user and header configuration data to create and assign `SavePhSpSD` sensitive detector instances to the main phase space logical volume and to additional volumes corresponding to specified Z-positions. Sensitive detectors are registered with the world geometry if valid configuration data is available.
 */
void SavePhSpConstruction::DefineSensitiveDetector(){

  /// !!!
  /// TODO: the following definition should be moved to the final/specific model definition
  ///
  /// TODO: The below logic of creating SavePhSpSD instances has to be revised!!!
  /// !!!

  G4int id = 0;
  auto worldConstr = Service<GeoSvc>()->World();
  auto configSvc = Service<ConfigSvc>();
  const VecG4doubleWPtr& savePhSpUsrWPtr = configSvc->GetValue<VecG4doubleSPtr>("GeoSvc", "SavePhSpUsr");
  const VecG4doubleWPtr& savePhSpHeadWPtr = configSvc->GetValue<VecG4doubleSPtr>("GeoSvc", "SavePhSpHead");
  if(!savePhSpHeadWPtr.expired() && !savePhSpUsrWPtr.expired()) {
    auto savePhSpHeadSPtr = savePhSpHeadWPtr.lock();
    auto savePhSpUsrSPtr = savePhSpUsrWPtr.lock();
    if (savePhSpUsrSPtr->size() > 0) {
      auto sensDet = new SavePhSpSD(*savePhSpUsrSPtr, *savePhSpHeadSPtr, id++);
      worldConstr->SetSensitiveDetector("PhSpBoxLV", sensDet);
    }
    for (const auto &iPhspZPos : *savePhSpHeadSPtr) {
      auto sensDet = new SavePhSpSD(*savePhSpUsrSPtr, *savePhSpHeadSPtr, id++ );
      worldConstr->SetSensitiveDetector("phspBoxInHeadLV" + svc::to_string(iPhspZPos), sensDet);
    }
  }
}
