#include "DishCubePhantom.hh"
#include "G4SystemOfUnits.hh"
#include "Services.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4SubtractionSolid.hh"
#include "G4ProductionCuts.hh"
#include "DishCubePhantomSD.hh"

////////////////////////////////////////////////////////////////////////////////
///
DishCubePhantom::DishCubePhantom():VPatient("DishCubePhantom"){
}

////////////////////////////////////////////////////////////////////////////////
///
DishCubePhantom::~DishCubePhantom() {
  Destroy();
}

////////////////////////////////////////////////////////////////////////////////
///
void DishCubePhantom::WriteInfo() {
  // TODO implement me.
}

////////////////////////////////////////////////////////////////////////////////
///
void DishCubePhantom::Destroy() {
  G4cout << "[INFO]:: \tDestroing the DishCubePhantom volume. " << G4endl;
  auto phantomVolume = GetPhysicalVolume();
  if (phantomVolume) {
    delete phantomVolume;
    SetPhysicalVolume(nullptr);
  }
}

////////////////////////////////////////////////////////////////////////////////
///
void DishCubePhantom::Construct(G4VPhysicalVolume *parentWorld) {
  G4cout << "[INFO]:: DishCubePhantom construction... " << G4endl;
  auto PMMA = Service<ConfigSvc>()->GetValue<G4MaterialSPtr>("MaterialsSvc", "PMMA");

  auto upperDishHole  = new G4Tubs("UpperDishHole", 0,46.75,5.5,0,360);
  auto bottomDishHole = new G4Tubs("BottomDishHole",0,44.25,5.0,0,360);

  // parent is now environemt box being created at the level of Patient Geometry
  auto phantom0 = parentWorld->GetLogicalVolume()->GetSolid();
  auto phantom1 = new G4SubtractionSolid("DishCubePhantom1", phantom0, upperDishHole, nullptr, G4ThreeVector(-43.5*mm,(-50-1.5)*mm,-5.5*mm)); // create 1st upper dish hole
  auto phantom2 = new G4SubtractionSolid("DishCubePhantom2", phantom1, bottomDishHole,nullptr, G4ThreeVector(-43.5*mm,(-50-1.5)*mm, 5.0*mm)); // create 1st bottom dish hole

  auto phantom3 = new G4SubtractionSolid("DishCubePhantom3", phantom2, upperDishHole, nullptr, G4ThreeVector(-43.5*mm, (50-1.5)*mm,-5.5*mm)); // create 2nd upper dish hole
  auto phantom4 = new G4SubtractionSolid("DishCubePhantom4", phantom3, bottomDishHole, nullptr, G4ThreeVector(-43.5*mm, (50-1.5)*mm, 5.0*mm)); // create 2nd bottom dish hole

  auto phantom5 = new G4SubtractionSolid("DishCubePhantom5", phantom4, upperDishHole, nullptr, G4ThreeVector(43.1*mm, (-1.5)*mm,-5.5*mm)); // create 3nd upper dish hole
  auto phantom6 = new G4SubtractionSolid("DishCubePhantom6", phantom5, bottomDishHole, nullptr, G4ThreeVector(43.1*mm, (-1.5)*mm, 5.0*mm)); // create 3nd bottom dish hole

  auto phantomLV = new G4LogicalVolume(phantom6, PMMA.get(), "DishCubePhantomLV");

  // NOTE:
  // the placement of phantom center in global coordinate system is managed by PatientGeometry class
  // here we locate the phantom box in the center of envelope box created in PatientGeometry, 
  // hence centre set to (0,0,0):
  SetPhysicalVolume(new G4PVPlacement(nullptr, G4ThreeVector(0.,0.,0.), "DishCubePhantomPV", phantomLV, parentWorld, false, 0));

  // Region for cuts
  auto regVol = new G4Region("DishCubePhantomR");
  auto cuts = new G4ProductionCuts;
  cuts->SetProductionCut(1.0 * mm);
  regVol->SetProductionCuts(cuts);

  phantomLV->SetRegion(regVol);
  regVol->AddRootLogicalVolume(phantomLV);

}

////////////////////////////////////////////////////////////////////////////////
///
G4bool DishCubePhantom::Update() {
  // TODO implement me.
  return true;
}

////////////////////////////////////////////////////////////////////////////////
///
void DishCubePhantom::DefineSensitiveDetector(){
  if(m_patientSD.Get()==0){
    auto pv = GetPhysicalVolume();
    auto patientSD = new DishCubePhantomSD("DishCubePhantomSD");

    // The full tank scoring:
    // ________________________________________________________________________
    G4String hcName = "DishCubePhantom";
    auto envBox = dynamic_cast<G4Box*>(pv->GetLogicalVolume()->GetSolid());
    patientSD->AddScoringVolume(hcName,hcName,*envBox,10,10,20,pv->GetTranslation());

    // Dish 1 scoring:
    // ________________________________________________________________________
    // TODO..

    // Dish 2 scoring:
    // ________________________________________________________________________
    // TODO..

    // Dish 3 scoring:
    // ________________________________________________________________________
    // TODO..

    //
    VPatient::SetSensitiveDetector("SciSlicePhantomLV", patientSD);
  }
}


