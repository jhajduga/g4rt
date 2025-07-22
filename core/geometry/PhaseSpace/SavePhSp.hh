/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 10.04.2020
*
*/

#ifndef Dose3D_SAVEPHSP_HH
#define Dose3D_SAVEPHSP_HH
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

#include "ConfigSvc.hh"

///\class SavePhSp
///\brief The set phase space planes
class SavePhSp : public IPhysicalVolume, public Configurable {
  public:
  ///
  SavePhSp();

  ///
  ~SavePhSp();

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

  private:
  ///
  void Configure() override;

  ///
  std::vector<G4PVPlacement*> m_usrPhspPV;
};

#endif  // Dose3D_SAVEPHSP_HH
