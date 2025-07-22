/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 14.06.2021
*
*/

#ifndef SCISLICE_PHANTOM_HH
#define SCISLICE_PHANTOM_HH

#include "G4PVPlacement.hh"
#include "VPatient.hh"

///\class SciSlicePhantom
///\brief The Phantom filled with scintilator
class SciSlicePhantom : public VPatient {
  public:
    ///
    SciSlicePhantom();

    ///
    ~SciSlicePhantom();

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

};

#endif  // SCISLICE_PHANTOM_HH
