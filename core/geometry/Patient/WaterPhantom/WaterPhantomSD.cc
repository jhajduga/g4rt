#include "WaterPhantomSD.hh"
#include "WaterPhantom.hh"
#include "StepAnalysis.hh"
#include <G4VProcess.hh>
#include "Services.hh"
#include "PatientTrackInfo.hh"
#include "PrimaryParticleInfo.hh"

////////////////////////////////////////////////////////////////////////////////
///
WaterPhantomSD::WaterPhantomSD(const G4String& sdName):VPatientSD(sdName){}

////////////////////////////////////////////////////////////////////////////////
///
WaterPhantomSD::WaterPhantomSD(const G4String& sdName, const G4ThreeVector& centre)
  : VPatientSD(sdName,centre){}

////////////////////////////////////////////////////////////////////////////////
/// This method is being called for each G4Step in sensitive volume
/// Note: Handling the G4Step/Hits it's important that different logical volumes
///       cane share one SD object!
/**
 * @brief Processes a simulation step in the water phantom sensitive volume.
 *
 * This method handles a simulation step by ensuring the hit occurs within the
 * water phantom sensitive volume. When track analysis is enabled, it updates or
 * creates user track information associated with the step. It also processes hit
 * collections for each registered control point and collects step data when step
 * analysis is active.
 *
 * @param aStep The simulation step containing hit and track information.
 * @param [unused] Placeholder for touchable history; not used in this implementation.
 * @return true indicating that the hit was successfully processed.
 */
G4bool WaterPhantomSD::ProcessHits(G4Step* aStep, G4TouchableHistory*) {
  
  // The TouchableHistory is used to obtain the physical volume of the hit
  // i.e. get volume where G4Step is remember (Note: PostStep "belongs" to next volume,
  // hence we use PreStepPoint!
  auto theTouchable = dynamic_cast<const G4TouchableHistory *>(aStep->GetPreStepPoint()->GetTouchable());
  auto volumeName = theTouchable->GetVolume()->GetName();
  if (volumeName != "WaterPhantomPV")
    // LOGSVC_DEBUG("ProcessHits volume name ", volumeName);
  
  // ____________________________________________________________________________
  if(m_tracks_analysis){
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