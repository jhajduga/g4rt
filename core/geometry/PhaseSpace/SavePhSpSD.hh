/**
*
* \author B. Rachwal
* \date 06.05.2020
*
*/

#ifndef Dose3D_SAVEPHASESPACESD_HH
#define Dose3D_SAVEPHASESPACESD_HH

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

///\class SavePhSpSD
class SavePhSpSD : public G4VSensitiveDetector {
  public:
  SavePhSpSD(std::vector<G4double> phspUsr, std::vector<G4double> phspHead, G4int id_);

  ~SavePhSpSD() = default;

  G4bool ProcessHits(G4Step *aStep, G4TouchableHistory *ROHist);

  private:
  G4AnalysisManager *analysisManager;
  std::vector<G4double> phspPositions;
  std::map<G4int, G4int> particleCodesMapping;
  G4double killerPosition;
  G4int id;
};

#endif // Dose3D_SAVEPHASESPACESD_HH