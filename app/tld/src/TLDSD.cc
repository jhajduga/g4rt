#include "TLDSD.hh"
#include "TLD.hh"
#include "StepAnalysis.hh"
#include <G4VProcess.hh>
#include "Services.hh"
#include "PatientTrackInfo.hh"
#include "PrimaryParticleInfo.hh"

////////////////////////////////////////////////////////////////////////////////
///
TLDSD::TLDSD(const G4String& sdName, const G4ThreeVector& centre, G4int idX, G4int idY, G4int idZ)
:VPatientSD(sdName,centre){
  m_id_x = idX;
  m_id_y = idY;
  m_id_z = idZ;
}

////////////////////////////////////////////////////////////////////////////////
/// This method is being called for each G4Step in sensitive volume
G4bool TLDSD::ProcessHits(G4Step* aStep, G4TouchableHistory*) {
  // The TouchableHistory is used to obtain the physical volume of the hit
  // i.e. get volume where G4Step is remember 
  // Note: PostStep on the boundary "belongs" to next volume hence we use PreStepPoint!
  auto theTouchable = dynamic_cast<const G4TouchableHistory *>(aStep->GetPreStepPoint()->GetTouchable());
  auto volumeName = theTouchable->GetVolume()->GetName();
  if ( ! G4StrUtil::contains(volumeName, "TLD"))
    LOGSVC_DEBUG("ProcessHits volume name ", volumeName);
  
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
  auto thisSdHCNames = GetScoringVolumeNames(); // it may happen that there is more instances of this SD type, 
  // hence Hits Collections existing independent from this cell geometry. 
  for(const auto& hcName : ControlPoint::GetHitCollectionNames()){
    if(find(thisSdHCNames.begin(), thisSdHCNames.end(), hcName) != thisSdHCNames.end())
      ProcessHitsCollection(hcName,aStep); 
  }
  return true;
}