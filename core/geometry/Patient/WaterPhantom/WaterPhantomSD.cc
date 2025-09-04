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
   * @brief Construct a WaterPhantom sensitive detector at a given center.
   *
   * Delegates construction to VPatientSD(sdName, centre) and initializes
   * the internal voxel coordinate IDs (m_id_x, m_id_y, m_id_z) to zero.
   *
   * @param sdName Sensitive detector name.
   * @param centre Detector center position in world coordinates.
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
 * @brief Process a Geant4 step for the WaterPhantom sensitive detector.
 *
 * Determines the pre-step volume, optionally records per-track PatientTrackInfo,
 * dispatches the step to all configured hit collections, and optionally forwards
 * the step to the step analysis subsystem.
 *
 * If the RunSvc "StoreTracks" flag is enabled, the function ensures the track has
 * a PatientTrackInfo attached: it updates an existing one or allocates a new
 * PatientTrackInfo and attaches it to the track (the Geant4 kernel is expected
 * to clean up the allocated object when the track ends).
 *
 * If the RunSvc "StepAnalysis" flag is enabled, the step is passed to
 * StepAnalysis::GetInstance()->FillStep().
 *
 * @param aStep The current Geant4 step (PreStepPoint is used to determine the volume).
 * @return G4bool Always returns true.
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