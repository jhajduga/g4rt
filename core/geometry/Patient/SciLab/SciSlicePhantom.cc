#include "SciSlicePhantom.hh"
#include "G4SystemOfUnits.hh"
#include "Services.hh"
#include "G4Box.hh"
#include "G4ProductionCuts.hh"
#include "SciSlicePhantomSD.hh"

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs a SciSlicePhantom object with the default name.
 *
 * Initializes the phantom by invoking the base VPatient constructor with the name "SciSlicePhantom".
 */
SciSlicePhantom::SciSlicePhantom():VPatient("SciSlicePhantom") {}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Destroys the SciSlicePhantom object and releases associated resources.
 *
 * Calls the Destroy() method to clean up the phantom's physical volume and related resources upon object destruction.
 */
SciSlicePhantom::~SciSlicePhantom() {
  Destroy();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Placeholder for writing information about the SciSlicePhantom.
 *
 * Intended for future implementation to output or record details about the phantom volume.
 */
void SciSlicePhantom::WriteInfo() {
  // TODO Implement me.
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Destroys the SciSlicePhantom physical volume and releases associated resources.
 *
 * Deletes the phantom's physical volume if it exists and resets the internal pointer to nullptr.
 */
void SciSlicePhantom::Destroy() {
  G4cout << "[INFO]:: \tDestroing the SciSlicePhantom volume. " << G4endl;
  auto phantomVolume = GetPhysicalVolume();
  if (phantomVolume) {
    delete phantomVolume;
    SetPhysicalVolume(nullptr);
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs the SciSlicePhantom volume within the simulation world.
 *
 * Defines a box-shaped phantom made of PMMA material, creates its logical and physical volumes, and places it at the origin of the provided parent world volume. Assigns a dedicated Geant4 region with a 1 mm production cut to the phantom logical volume.
 *
 * @param parentWorld The parent world physical volume in which the phantom is placed.
 */
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
/**
 * @brief Placeholder method for updating the SciSlicePhantom.
 *
 * Currently unimplemented; always returns true.
 *
 * @return G4bool Always returns true.
 */
G4bool SciSlicePhantom::Update() {
  // TODO implement me.
  return true;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Attaches a sensitive detector to the SciSlicePhantom volume if not already present.
 *
 * Creates and configures a `SciSlicePhantomSD` sensitive detector, adds the phantom's box-shaped scoring volume with specified segmentation, and registers the detector with the logical volume. No action is taken if the sensitive detector already exists.
 */
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


