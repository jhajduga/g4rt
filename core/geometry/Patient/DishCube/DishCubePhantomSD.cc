#include "DishCubePhantomSD.hh"
#include "DishCubePhantom.hh"
#include "StepAnalysis.hh"
#include <G4VProcess.hh>
#include "Services.hh"
#include "PatientTrackInfo.hh"

////////////////////////////////////////////////////////////////////////////////
///
DishCubePhantomSD::DishCubePhantomSD(const G4String& sdName):VPatientSD(sdName){}

////////////////////////////////////////////////////////////////////////////////
/// This method is being called for each G4Step in sensitive volume
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