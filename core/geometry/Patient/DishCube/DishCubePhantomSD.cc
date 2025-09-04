#include "DishCubePhantomSD.hh"
#include "DishCubePhantom.hh"
#include "StepAnalysis.hh"
#include <G4VProcess.hh>
#include "Services.hh"
#include "PatientTrackInfo.hh"

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Create a DishCubePhantomSD sensitive detector with the given name.
 *
 * Constructs the sensitive detector and forwards the provided name to the VPatientSD base class.
 *
 * @param sdName Name assigned to this sensitive detector instance.
 */
DishCubePhantomSD::DishCubePhantomSD(const G4String& sdName):VPatientSD(sdName){}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Handle a Geant4 step occurring inside the DishCube phantom sensitive volume.
 *
 * Updates per-track user information for the traversing particle (creating and attaching
 * a PatientTrackInfo if none exists), processes all configured hit collections for this step,
 * and—if enabled via configuration—records the step for offline analysis.
 *
 * This function has the side effect of allocating and attaching a PatientTrackInfo to the
 * track when one is not already present. It may also emit a debug message when the step's
 * pre-step volume name is not "DishCubePhantomPV".
 *
 * @param aStep The current Geant4 step in the sensitive detector.
 * @return G4bool Always returns true to indicate the hit was processed.
 */
G4bool DishCubePhantomSD::ProcessHits(G4Step* aStep, G4TouchableHistory*) {
  
  // The TouchableHistory is used to obtain the physical volume of the hit
  // i.e. get volume where G4Step is remember (Note: PostStep "belongs" to next volume,
  // hence we use PreStepPoint!
  auto theTouchable = dynamic_cast<const G4TouchableHistory *>(aStep->GetPreStepPoint()->GetTouchable());
  auto volumeName = theTouchable->GetVolume()->GetName();
  if (volumeName != "DishCubePhantomPV")
    G4cout << "[debug]:: ProcessHits volume name " << volumeName << G4endl;
  
  // ____________________________________________________________________________
  auto aTrack = aStep->GetTrack();
  auto trackInfo = aTrack->GetUserInformation();
  if(trackInfo){
    dynamic_cast<PatientTrackInfo*>(trackInfo)->FillInfo(aStep);
  }
  else{
    trackInfo = new PatientTrackInfo(); // it's being cleaned up by kernel when track ends it's life
    dynamic_cast<PatientTrackInfo*>(trackInfo)->FillInfo(aStep);
    aTrack->SetUserInformation(trackInfo);
  }

  // ____________________________________________________________________________
  for(const auto& hcName : ControlPoint::GetHitCollectionNames())
    ProcessHitsCollection(hcName,aStep); 

  // ____________________________________________________________________________
  // Collect all hits in this detector volume
  if (Service<ConfigSvc>()->GetValue<bool>("RunSvc", "StepAnalysis"))
    StepAnalysis::GetInstance()->FillStep(aStep);

  return true;
}