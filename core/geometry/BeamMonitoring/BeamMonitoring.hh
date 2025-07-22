/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 19.05.2020
*
*/

#ifndef Dose3D_BEAMMONITORING_HH
#define Dose3D_BEAMMONITORING_HH
//shapes
#include "G4Box.hh"
#include "G4Tubs.hh"

#include "G4LogicalVolume.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4VPhysicalVolume.hh"
#include "G4VisAttributes.hh"

#include "G4ProductionCuts.hh"
#include "G4SDManager.hh"
#include "IPhysicalVolume.hh"
#include "Configurable.hh"
#include "G4Cache.hh"
#include "ConfigSvc.hh"

class BeamMonitoringSD;

class BeamMonitoring : public IPhysicalVolume, public Configurable {
  public:
  ///
  BeamMonitoring();

  ///
  ~BeamMonitoring();

  ///
  void Construct(G4VPhysicalVolume *parentPV) override;

  ///
  void Destroy() override;

  ///
  G4bool Update() override;

  ///
  void Reset() override { G4cout << "Implement me." << G4endl; }

  ///
  void WriteInfo() override;

  ///
  void DefaultConfig(const std::string &unit) override;

  ///
  void DefineSensitiveDetector();

  private:
  ///
  void Configure() override;

  ///
  std::vector<G4PVPlacement*> m_scoringPlanesPV;

  /// Pointer to the sensitive detectors, wrapped into the MT service
  G4Cache<BeamMonitoringSD*> m_beamMonitoringSD;
};

#endif  // Dose3D_BEAMMONITORING_HH
