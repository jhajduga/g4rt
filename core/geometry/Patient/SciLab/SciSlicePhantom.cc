#include "SciSlicePhantom.hh"
#include "G4SystemOfUnits.hh"
#include "Services.hh"
#include "G4Box.hh"
#include "G4ProductionCuts.hh"
#include "SciSlicePhantomSD.hh"

////////////////////////////////////////////////////////////////////////////////
///
SciSlicePhantom::SciSlicePhantom():VPatient("SciSlicePhantom") {}

////////////////////////////////////////////////////////////////////////////////
///
SciSlicePhantom::~SciSlicePhantom() {
  Destroy();
}

////////////////////////////////////////////////////////////////////////////////
///
void SciSlicePhantom::WriteInfo() {
  // TODO Implement me.
}

////////////////////////////////////////////////////////////////////////////////
///
void SciSlicePhantom::Destroy() {
  G4cout << "[INFO]:: \tDestroing the SciSlicePhantom volume. " << G4endl;
  auto phantomVolume = GetPhysicalVolume();
  if (phantomVolume) {
    delete phantomVolume;
    SetPhysicalVolume(nullptr);
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void SciSlicePhantom::Construct(G4VPhysicalVolume *parentWorld) {
  G4cout << "[INFO]:: SciSlicePhantom construction... " << G4endl;
  auto medium = Service<ConfigSvc>()->GetValue<G4MaterialSPtr>("MaterialsSvc", "PMMA");

  auto size =G4ThreeVector(10. *mm, 2. * mm, 2. * mm);
  auto phantomBox = new G4Box("SciSlicePhantomBox", size.getX()/2., size.getY()/2., size.getZ()/2.);
  auto phantomLV = new G4LogicalVolume(phantomBox, medium.get(), "SciSlicePhantomLV");

  // the placement of phantom center in global coordinate system is managed by PatientGeometry class
  // here we locate the phantom box in the center of envelope box created in PatientGeometry, 
  // hence centre set to (0,0,0):
  SetPhysicalVolume(new G4PVPlacement(nullptr, G4ThreeVector(0.,0.,0.), "SciSlicePhantomPV", phantomLV, parentWorld, false, 0));

  // Region for cuts
  auto regVol = new G4Region("SciSlicePhantomR");
  auto cuts = new G4ProductionCuts;
  cuts->SetProductionCut(1.0 * mm);
  regVol->SetProductionCuts(cuts);

  phantomLV->SetRegion(regVol);
  regVol->AddRootLogicalVolume(phantomLV);

}

////////////////////////////////////////////////////////////////////////////////
///
G4bool SciSlicePhantom::Update() {
  // TODO implement me.
  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
void SciSlicePhantom::DefineSensitiveDetector(){
  if(m_patientSD.Get()==0){
    G4cout << "[INFO]:: SciSlicePhantom: creating the Sensitive Detector!" << G4endl;

    auto pv = GetPhysicalVolume();
    auto patientSD = new SciSlicePhantomSD("SciSlicePhantomSD");

    // ________________________________________________________________________
    G4String hcName = "SciSlicePhantom";
    auto envBox = dynamic_cast<G4Box*>(pv->GetLogicalVolume()->GetSolid());
    patientSD->AddScoringVolume(hcName,hcName,*envBox,20,4,4,pv->GetTranslation());
    
    //
    VPatient::SetSensitiveDetector("SciSlicePhantomLV", patientSD);
  }
}


