/**
*
* \author B. Rachwal
* \date 19.05.2020
*
*/

#ifndef Dose3D_BEAMMONITORING_SD_HH
#define Dose3D_BEAMMONITORING_SD_HH

#include <G4UnitsTable.hh>
#include <vector>
#include "ConfigSvc.hh"
#include "G4Electron.hh"
#include "G4Event.hh"
#include "G4Gamma.hh"
#include "G4Neutron.hh"
#include "G4ParticleDefinition.hh"
#include "G4Positron.hh"
#include "G4Proton.hh"
#include "G4Step.hh"
#include "G4TouchableHistory.hh"
#include "G4VProcess.hh"
#include "G4VSensitiveDetector.hh"
#include "G4VTouchable.hh"
#include "G4AnalysisManager.hh"
#include "globals.hh"

class G4Step;

///\class BeamMonitoringSD
class BeamMonitoringSD : public G4VSensitiveDetector {
  public:
  ///
  BeamMonitoringSD(const G4String& sdName);

  ///
  ~BeamMonitoringSD() = default;

  ///
  G4bool ProcessHits(G4Step*, G4TouchableHistory*) override;

  private:
  ///
  G4AnalysisManager *analysisManager;

  ///
  G4int m_idx_z = -1;
};

#endif // Dose3D_BEAMMONITORING_SD_HH