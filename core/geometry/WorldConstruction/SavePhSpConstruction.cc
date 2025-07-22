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
///
SavePhSpConstruction* SavePhSpConstruction::GetInstance() {
  static SavePhSpConstruction instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
///
bool SavePhSpConstruction::ModelSetup() {
  //auto configSvc = Service<ConfigSvc>();
  // NOTE: Currently there is only one model of SavePhSp planes
  m_savePhSp = new SavePhSp();
  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
void SavePhSpConstruction::Destroy() {
  if (PVPhmWorld) {
    if (m_savePhSp) m_savePhSp->Destroy();
    delete PVPhmWorld;
    PVPhmWorld = nullptr;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void SavePhSpConstruction::Construct(G4VPhysicalVolume *parentPV) {
  // A call to select the right model
  if (ModelSetup()) {
    // NOTE: Do not create the envelop box - the phase space planes are being created
    //       in the main world box:
    m_savePhSp->Construct(parentPV);
  }
}

////////////////////////////////////////////////////////////////////////////////
///
G4bool SavePhSpConstruction::Update() {
  if (m_savePhSp) {
    if (!m_savePhSp->Update()) return false;
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
void SavePhSpConstruction::WriteInfo() {
  if(m_savePhSp) m_savePhSp->WriteInfo();
}

////////////////////////////////////////////////////////////////////////////////
///
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
