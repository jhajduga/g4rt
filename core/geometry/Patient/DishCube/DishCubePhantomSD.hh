/**
*
* \author B. Rachwal (brachwal [at] agh.edu.pl)
* \date 10.09.2021
*
*/

#ifndef DISHCUBE_PHANTOM_SD_HH
#define DISHCUBE_PHANTOM_SD_HH

#include "VPatientSD.hh"

class DishCubePhantomSD : public VPatientSD {

  public:
    ///
    DishCubePhantomSD(const G4String& sdName);

    ///
    ~DishCubePhantomSD() = default;

    ///
    G4bool ProcessHits(G4Step*, G4TouchableHistory*) override;

};

#endif //DISHCUBE_PHANTOM_SD_HH
