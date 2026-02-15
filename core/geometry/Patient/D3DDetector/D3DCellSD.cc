#include "D3DCellSD.hh"
#include "D3DCell.hh"
#include "StepAnalysis.hh"
#include <G4VProcess.hh>
#include "Services.hh"
#include "PatientTrackInfo.hh"
#include "PrimaryParticleInfo.hh"

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Create a 3D sensitive detector cell with a name, position, and grid indices.
 *
 * Constructs the cell by forwarding the detector name and center to the base VPatientSD
 * and stores integer indices identifying the cell's position in the 3D grid.
 *
 * @param sdName Sensitive detector name.
 * @param centre Cell center position in global coordinates.
 * @param idX Cell index along the X axis.
 * @param idY Cell index along the Y axis.
 * @param idZ Cell index along the Z axis.
 */
D3DCellSD::D3DCellSD(const G4String& sdName, const G4ThreeVector& centre, G4int idX, G4int idY, G4int idZ)
:VPatientSD(sdName,centre){
  m_id_x = idX;
  m_id_y = idY;
  m_id_z = idZ;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Handle a Geant4 step occurring inside this 3D sensitive detector cell.
 *
 * Processes aStep for scoring: optionally updates or attaches PatientTrackInfo to the current track
 * when StoreTracks is enabled, and forwards the step to each hit collection owned by this SD
 * (calls ProcessHitsCollection for matching hit collection names).
 *
 * The step's PreStepPoint touchable is used to identify the physical volume containing the step.
 *
 * @param aStep The current Geant4 step within the sensitive volume.
 * @return G4bool True when the step was processed.
 */
G4bool D3DCellSD::ProcessHits(G4Step* aStep, G4TouchableHistory*) {
  // The TouchableHistory is used to obtain the physical volume of the hit
  // i.e. get volume where G4Step is remember 
  // Note: PostStep on the boundary "belongs" to next volume hence we use PreStepPoint!
  auto theTouchable = dynamic_cast<const G4TouchableHistory *>(aStep->GetPreStepPoint()->GetTouchable());
  auto volumeName = theTouchable->GetVolume()->GetName();
  if ( ! G4StrUtil::contains(volumeName, "D3D"))
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