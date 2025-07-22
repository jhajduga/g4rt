/**
*
* \author B. Rachwal (brachwal [at] agh.edu.pl)
* \date 03.12.2020
*
*/

#ifndef PHSIAEAPRIMARYGENERATOR_HH
#define PHSIAEAPRIMARYGENERATOR_HH

#include "G4VPrimaryGenerator.hh"
#include "G4String.hh"
#include <vector>

class G4IAEAphspReader;
class G4PrimaryVertex;

///\class IaeaPrimaryGenerator
class IaeaPrimaryGenerator : public G4VPrimaryGenerator {
  public:
    /****
 * @brief Default constructor is deleted to prevent instantiation without a phase space file.
 */
    IaeaPrimaryGenerator() = delete;

    ///
    explicit IaeaPrimaryGenerator(const G4String& fileName);

    /**
 * @brief Destroys the IaeaPrimaryGenerator instance.
 */
    ~IaeaPrimaryGenerator() = default;

    ///
    void GeneratePrimaryVertex(G4Event*) override;
    std::vector<G4PrimaryVertex*> GeneratePrimaryVertexVector(G4Event *evt);

  private:
    ///
    static G4IAEAphspReader* m_iaeaFileReader;

    ///
    G4double m_phspShiftZ = 0;
};

#endif  // PHSIAEAPRIMARYGENERATOR_HH
