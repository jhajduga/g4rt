#include "TLDSD.hh"
#include "TLD.hh"
#include "StepAnalysis.hh"
#include <G4VProcess.hh>
#include "Services.hh"
#include "PatientTrackInfo.hh"
#include "PrimaryParticleInfo.hh"

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Create a TLDSD sensitive detector.
 *
 * Constructs a TLDSD and initializes its base VPatientSD with the given
 * name and center position. Stores the integer 3‑D identifiers that locate
 * this detector instance within the TLD array.
 *
 * @param sdName Detector name used by the base VPatientSD.
 * @param centre Center position of the detector in world coordinates.
 * @param idX Integer identifier along the X index of the detector array.
 * @param idY Integer identifier along the Y index of the detector array.
 * @param idZ Integer identifier along the Z index of the detector array.
 */
TLDSD::TLDSD(const G4String& sdName, const G4ThreeVector& centre, G4int idX, G4int idY, G4int idZ)
:VPatientSD(sdName,centre){
  m_id_x = idX;
  m_id_y = idY;
  m_id_z = idZ;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Handle a Geant4 step occurring inside this sensitive detector.
 *
 * Processes aStep by:
 * - Inspecting the pre-step touchable to determine the containing volume.
 * - If configured (RunSvc.StoreTracks), ensuring a PatientTrackInfo is attached to the track and updating it with the step.
 * - Iterating the global hit-collection names and calling ProcessHitsCollection for any collection that belongs to this detector instance (based on GetScoringVolumeNames()).
 *
 * The function may attach a newly created PatientTrackInfo to the track (the track/user-information lifetime is managed by Geant4). It always returns true on completion.
 *
 * @param aStep The current Geant4 step to process (uses PreStepPoint for volume lookup).
 * @return G4bool True upon successful processing.
 */
G4bool TLDSD::ProcessHits(G4Step* aStep, G4TouchableHistory*) {
  // The TouchableHistory is used to obtain the physical volume of the hit
  // i.e. get volume where G4Step is remember 
  // Note: PostStep on the boundary "belongs" to next volume hence we use PreStepPoint!
  auto theTouchable = dynamic_cast<const G4TouchableHistory *>(aStep->GetPreStepPoint()->GetTouchable());
  auto volumeName = theTouchable->GetVolume()->GetName();
  if ( ! G4StrUtil::contains(volumeName, "TLD"))
    DEBUG_GEO("ProcessHits volume name ", volumeName);
  
  // ____________________________________________________________________________
  if (Service<ConfigSvc>()->GetValue<bool>("RunSvc", "StoreTracks")) {
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
  auto thisSdHCNames = GetScoringVolumeNames(); // it may happen that there is more instances of this SD type, 
  // hence Hits Collections existing independent from this cell geometry. 
  for(const auto& hcName : ControlPoint::GetHitCollectionNames()){
    if(find(thisSdHCNames.begin(), thisSdHCNames.end(), hcName) != thisSdHCNames.end())
      ProcessHitsCollection(hcName,aStep); 
  }
  return true;
}