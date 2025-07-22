#include "TLDSD.hh"
#include "TLD.hh"
#include "StepAnalysis.hh"
#include <G4VProcess.hh>
#include "Services.hh"
#include "PatientTrackInfo.hh"
#include "PrimaryParticleInfo.hh"

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs a TLDSD sensitive detector with a specified name, center position, and spatial identifiers.
 *
 * @param sdName Name of the sensitive detector.
 * @param centre Center position of the detector in 3D space.
 * @param idX X-coordinate identifier for the detector.
 * @param idY Y-coordinate identifier for the detector.
 * @param idZ Z-coordinate identifier for the detector.
 */
TLDSD::TLDSD(const G4String& sdName, const G4ThreeVector& centre, G4int idX, G4int idY, G4int idZ)
:VPatientSD(sdName,centre){
  m_id_x = idX;
  m_id_y = idY;
  m_id_z = idZ;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Processes a Geant4 step within the sensitive detector volume.
 *
 * For each step occurring in the sensitive volume, updates track information if configured, and processes relevant hit collections associated with this detector instance.
 *
 * @param aStep The current Geant4 step to process.
 * @return G4bool Always returns true to indicate successful processing.
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