/**
*
* \author B. Rachwal (brachwal [at] agh.edu.pl)
* \date 10.04.2020
*
*/

#ifndef WATER_PHANTOM_SD_HH
#define WATER_PHANTOM_SD_HH

#include "VPatientSD.hh"

class WaterPhantomSD : public VPatientSD {

  public:
  ///
  WaterPhantomSD(const G4String& sdName);

  //
  WaterPhantomSD(const G4String& sdName, const G4ThreeVector& centre);

  ///
  ~WaterPhantomSD() = default;

  ///
  G4bool ProcessHits(G4Step*, G4TouchableHistory*) override;

};

#endif //WATER_PHANTOM_SD_HH
