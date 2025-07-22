#include "SciSlicePhantomSD.hh"
#include "SciSlicePhantom.hh"
#include "StepAnalysis.hh"
#include <G4VProcess.hh>
#include "Services.hh"
#include "PatientTrackInfo.hh"

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs a SciSlicePhantomSD sensitive detector with the specified name.
 *
 * Initializes the sensitive detector by passing the given name to the base class VPatientSD.
 *
 * @param sdName Name of the sensitive detector.
 */
SciSlicePhantomSD::SciSlicePhantomSD(const G4String& sdName):VPatientSD(sdName){}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Processes a Geant4 step within the sensitive detector volume.
 *
 * Associates step information with the current track's user data, processes all relevant hit collections for the step, and optionally records step analysis data if enabled in the configuration.
 *
 * @param aStep The current Geant4 step occurring in the sensitive detector.
 * @return G4bool Always returns true to indicate the hit was processed.
 */
G4bool SciSlicePhantomSD::ProcessHits(G4Step* aStep, G4TouchableHistory*) {
  
  // The TouchableHistory is used to obtain the physical volume of the hit
  // i.e. get volume where G4Step is remember (Note: PostStep "belongs" to next volume,
  // hence we use PreStepPoint!
  auto theTouchable = dynamic_cast<const G4TouchableHistory *>(aStep->GetPreStepPoint()->GetTouchable());
  auto volumeName = theTouchable->GetVolume()->GetName();
  //G4cout << "[debug]:: I'm in  " << volumeName << G4endl;

  if (volumeName != "SciSlicePhantomPV")
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