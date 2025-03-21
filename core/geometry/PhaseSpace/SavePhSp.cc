#include "SavePhSp.hh"
#include "G4SystemOfUnits.hh"
#include "Services.hh"

////////////////////////////////////////////////////////////////////////////////
///
SavePhSp::SavePhSp(): IPhysicalVolume("SavePhSp"), Configurable("SavePhSp"){
  Configure();
}

////////////////////////////////////////////////////////////////////////////////
///
SavePhSp::~SavePhSp() {
  configSvc()->Unregister(thisConfig()->GetName());
  Destroy();
}

////////////////////////////////////////////////////////////////////////////////
///
void SavePhSp::Configure() {
  G4cout << "\n\n[INFO]::  Configuring the " << thisConfig()->GetName() << G4endl;

  Configurable::DefaultConfig();   // setup the default configuration for all defined units/parameters
  Configurable::PrintConfig();
}

////////////////////////////////////////////////////////////////////////////////
///
void SavePhSp::DefaultConfig(const std::string &unit) {

  // Volume name
  if (unit.compare("Label") == 0)
    thisConfig()->SetValue(unit, std::string("Save phase space"));
}

////////////////////////////////////////////////////////////////////////////////
///
void SavePhSp::WriteInfo() {
  /// implement me.
}

////////////////////////////////////////////////////////////////////////////////
///
void SavePhSp::Destroy() {
  G4cout << "[INFO]:: \tDestroing the SavePhSp volume. " << G4endl;
  for(auto& phsp : m_usrPhspPV){
    delete phsp;
  }
}

////////////////////////////////////////////////////////////////////////////////
///
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
///
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


