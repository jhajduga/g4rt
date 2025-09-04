#include "BeamMonitoringSD.hh"
#include "BeamAnalysis.hh"
#include "G4SystemOfUnits.hh"
#include "G4AutoLock.hh"

namespace { G4Mutex sdMutex = G4MUTEX_INITIALIZER; }

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Create a BeamMonitoringSD sensitive detector with the given name.
 *
 * Forwards the name to the G4VSensitiveDetector base and initializes the
 * analysisManager member by obtaining the G4AnalysisManager singleton.
 * The acquisition is performed while holding the internal sdMutex to ensure
 * thread-safe initialization.
 *
 * @param sdName The sensitive detector name exposed to Geant4.
 */
BeamMonitoringSD::BeamMonitoringSD(const G4String& sdName) : G4VSensitiveDetector(sdName) {

  G4AutoLock lock(&sdMutex);

  analysisManager = G4AnalysisManager::Instance();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Processes a simulation step within the sensitive detector and records particle data.
 *
 * Extracts the scoring plane identifier from the volume name associated with the current step and passes the step and identifier to the BeamAnalysis singleton for particle data recording.
 *
 * @param step The current simulation step within the sensitive detector.
 * @return G4bool Always returns true to indicate successful hit processing.
 */
G4bool BeamMonitoringSD::ProcessHits(G4Step *step, G4TouchableHistory *) {

  // if (step->GetPreStepPoint()->GetStepStatus() == G4StepStatus::fGeomBoundary) {
    auto theTouchable = dynamic_cast<const G4TouchableHistory *>(step->GetPreStepPoint()->GetTouchable());
    std::string volumeName = theTouchable->GetVolume()->GetName();
    auto scoringPlaneIdStr = volumeName.substr(volumeName.find('_')+1,volumeName.length());
    auto scoringPlaneId = std::stoi(scoringPlaneIdStr);
    // G4cout << "[DEBUG]:: BeamMonitoringSD::volumeName " << volumeName << "  id "<< scoringPlaneId << G4endl;
    BeamAnalysis::GetInstance()->FillParticles(step,scoringPlaneId);
  // }
  return true;
}