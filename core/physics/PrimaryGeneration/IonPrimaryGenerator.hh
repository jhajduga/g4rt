/**
*
* \author B. Rachwal (brachwal [at] agh.edu.pl)
* \date 15.06.2021
*
*/

#ifndef ION_PRIMARYGENERATOR_HH
#define ION_PRIMARYGENERATOR_HH

#include "G4GeneralParticleSource.hh"

///\class IonPrimaryGenerator
class IonPrimaryGenerator : public G4GeneralParticleSource {
  public:
    ///
    IonPrimaryGenerator() = default;

    ///
    ~IonPrimaryGenerator() = default;

    ///
    void GeneratePrimaryVertex(G4Event*) override;

    /// Z - AtomicNumber
    /// A - AtomicMass
    /// Q - Charge of Ion (in unit of e)
    /// E - Excitation energy (in keV)
    G4int AddSource(G4int m_Z, G4int m_A, G4int m_Q, G4double m_E, G4double m_Strength);

  private:
    ///
};

#endif  // ION_PRIMARYGENERATOR_HH
