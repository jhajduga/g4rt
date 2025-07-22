/**
*
* \author B. Rachwal (brachwal [at] agh.edu.pl)
* \date 21.06.2024
*
*/

#ifndef TLD_SD_HH
#define TLD_SD_HH

#include "VPatientSD.hh"

class TLDSD : public VPatientSD {

  public:
    ///
    TLDSD(const G4String& sdName, const G4ThreeVector& centre, G4int idX, G4int idY, G4int idZ);

    ///
    ~TLDSD() = default;

    ///
    G4bool ProcessHits(G4Step*, G4TouchableHistory*) override;

};

#endif //TLD_SD_HH
