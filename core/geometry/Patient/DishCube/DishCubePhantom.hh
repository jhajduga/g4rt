/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 10.09.2021
*
*/

#ifndef DISHCUBE_PHANTOM_HH
#define DISHCUBE_PHANTOM_HH

#include "G4PVPlacement.hh"
#include "VPatient.hh"

///\class DishCubePhantom
///\brief The Phantom filled with scintilator
class DishCubePhantom : public VPatient {
  public:
    ///
    DishCubePhantom();

    ///
    ~DishCubePhantom();

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
    void DefineSensitiveDetector() override;

    ///
    void ParseTomlConfig() override {}


  private:

};

#endif  // DISHCUBE_PHANTOM_HH
