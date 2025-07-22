#include "BeamMonitoringSD.hh"
#include "BeamAnalysis.hh"
#include "G4SystemOfUnits.hh"
#include "G4AutoLock.hh"

namespace { G4Mutex sdMutex = G4MUTEX_INITIALIZER; }

////////////////////////////////////////////////////////////////////////////////
///
BeamMonitoringSD::BeamMonitoringSD(const G4String& sdName) : G4VSensitiveDetector(sdName) {

  G4AutoLock lock(&sdMutex);

  analysisManager = G4AnalysisManager::Instance();
}

////////////////////////////////////////////////////////////////////////////////
///
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