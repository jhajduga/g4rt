/**
*
* \author B. Rachwal (brachwal [at] agh.edu.pl)
* \date 14.06.2021
*
*/

#ifndef SCISLICE_PHANTOM_SD_HH
#define SCISLICE_PHANTOM_SD_HH

#include "VPatientSD.hh"

class SciSlicePhantomSD : public VPatientSD {

  public:
    ///
    SciSlicePhantomSD(const G4String& sdName);

    ///
    ~SciSlicePhantomSD() = default;

    ///
    G4bool ProcessHits(G4Step*, G4TouchableHistory*) override;
};

#endif //SCISLICE_PHANTOM_SD_HH
