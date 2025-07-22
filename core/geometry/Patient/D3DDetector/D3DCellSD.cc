#include "D3DCellSD.hh"
#include "D3DCell.hh"
#include "StepAnalysis.hh"
#include <G4VProcess.hh>
#include "Services.hh"
#include "PatientTrackInfo.hh"
#include "PrimaryParticleInfo.hh"

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs a 3D sensitive detector cell with specified name, center, and grid indices.
 *
 * Initializes the detector cell with a unique name, its center position in 3D space, and integer indices identifying its location within the grid.
 *
 * @param sdName Name of the sensitive detector.
 * @param centre Center position of the cell in 3D space.
 * @param idX X-axis index of the cell in the grid.
 * @param idY Y-axis index of the cell in the grid.
 * @param idZ Z-axis index of the cell in the grid.
 */
D3DCellSD::D3DCellSD(const G4String& sdName, const G4ThreeVector& centre, G4int idX, G4int idY, G4int idZ)
:VPatientSD(sdName,centre){
  m_id_x = idX;
  m_id_y = idY;
  m_id_z = idZ;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Processes a Geant4 step occurring in this sensitive detector cell.
 *
 * Updates track information if configured, and processes hits for all relevant scoring volumes associated with this detector cell. Returns true to indicate successful handling of the step.
 *
 * @param aStep The current Geant4 step within the sensitive volume.
 * @return G4bool True if the hit was processed successfully.
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