/**
*
* \author B. Rachwal (brachwal [at] agh.edu.pl)
* \date 10.09.2021
*
*/

#ifndef D3D_CELL_SD_HH
#define D3D_CELL_SD_HH

#include "VPatientSD.hh"

class D3DCellSD : public VPatientSD {

  public:
    ///
    D3DCellSD(const G4String& sdName, const G4ThreeVector& centre, G4int idX, G4int idY, G4int idZ);

    ///
    ~D3DCellSD() = default;

    ///
    G4bool ProcessHits(G4Step*, G4TouchableHistory*) override;

};

#endif //D3D_CELL_SD_HH
