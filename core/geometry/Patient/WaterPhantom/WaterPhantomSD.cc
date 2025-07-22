#include "WaterPhantomSD.hh"
#include "WaterPhantom.hh"
#include "StepAnalysis.hh"
#include <G4VProcess.hh>
#include "Services.hh"
#include "PatientTrackInfo.hh"
#include "PrimaryParticleInfo.hh"

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs a WaterPhantomSD sensitive detector with the specified name.
 *
 * Initializes the sensitive detector by passing the name to the base VPatientSD class.
 */
WaterPhantomSD::WaterPhantomSD(const G4String& sdName):VPatientSD(sdName){}

////////////////////////////////////////////////////////////////////////////////
/**
   * @brief Constructs a WaterPhantomSD sensitive detector with a specified name and center position.
   *
   * Initializes the detector's internal coordinate IDs to zero.
   *
   * @param sdName Name of the sensitive detector.
   * @param centre Center position of the detector in 3D space.
   */
WaterPhantomSD::WaterPhantomSD(const G4String& sdName, const G4ThreeVector& centre)
  : VPatientSD(sdName,centre){
    m_id_x = 0;
    m_id_y = 0;
    m_id_z = 0;
  }

////////////////////////////////////////////////////////////////////////////////
/// This method is being called for each G4Step in sensitive volume
/// Note: Handling the G4Step/Hits it's important that different logical volumes
///       cane share one SD object!
/**
 * @brief Processes a Geant4 step within the water phantom sensitive detector.
 *
 * Handles hit processing for each step in the water phantom volume, including track information management, hit collection processing, and optional step analysis based on configuration settings.
 *
 * @param aStep The current Geant4 step to process.
 * @return G4bool Always returns true to indicate successful processing.
 */
G4bool WaterPhantomSD::ProcessHits(G4Step* aStep, G4TouchableHistory*) {
  
  // The TouchableHistory is used to obtain the physical volume of the hit
  // i.e. get volume where G4Step is remember (Note: PostStep "belongs" to next volume,
  // hence we use PreStepPoint!
  auto theTouchable = dynamic_cast<const G4TouchableHistory *>(aStep->GetPreStepPoint()->GetTouchable());
  auto volumeName = theTouchable->GetVolume()->GetName();
  if (volumeName != "WaterPhantomPV")
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
  for(const auto& hcName : ControlPoint::GetHitCollectionNames())
    ProcessHitsCollection(hcName,aStep); 

  // ____________________________________________________________________________
  // Collect all hits in this detector volume
  if (Service<ConfigSvc>()->GetValue<bool>("RunSvc", "StepAnalysis"))
    StepAnalysis::GetInstance()->FillStep(aStep);

  return true;
}