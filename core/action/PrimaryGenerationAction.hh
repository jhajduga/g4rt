/**
*
* \author B.Rachwal (brachwal [at] agh.edu.pl)
* \date 08.04.2020
*
*/

#ifndef PRIMARYGENERATIONACTION_H
#define PRIMARYGENERATIONACTION_H


//#include "G4RunManager.hh"
#include "G4VUserPrimaryGeneratorAction.hh"
#include "G4String.hh"
#include "G4RotationMatrix.hh"

class G4Event;
class G4PrimaryVertex;
class G4VPrimaryGenerator;

enum class PrimaryGeneratorType : G4int {
  PhspIAEA = 1,   ///< IAEA binary file format
  GPS = 2,        ///< Geant4 GPS particle source (based on macro files interface)
  IonGPS = 3      ///< Ion GPS-based particle source
};

///\class PrimaryGenerationAction
class PrimaryGenerationAction : public G4VUserPrimaryGeneratorAction {
  public:
    ///
    PrimaryGenerationAction();

    ///
    ~PrimaryGenerationAction();

    ///
    void GeneratePrimaries(G4Event *anEvent) override;

    /**
 * @brief Set the global rotation matrix used when generating primary particles.
 *
 * Provide a pointer to a G4RotationMatrix that will be applied to generated primaries.
 * Passing nullptr clears the rotation (no rotation will be applied).
 *
 * @param rotMatrix Pointer to the rotation matrix to use (may be nullptr).
 */
    static void SetRotation(G4RotationMatrix* rotMatrix) { m_rotation_matrix = rotMatrix; }

    /**
 * @brief Set the global source-to-isocentre distance used when generating primaries.
 *
 * This static setter updates the distance from the radiation source to the isocentre
 * that will be applied by the primary generation logic.
 *
 * @param distance Distance from the source to the isocentre (in the simulation's length units).
 */
    static void SetSID(G4double distance) { m_source_isocentre_distance = distance; }

  private:

    ///
    PrimaryGeneratorType m_generatorType = PrimaryGeneratorType::PhspIAEA;

    ///
    G4VPrimaryGenerator* m_primaryGenerator = nullptr;

    ///
    void SetPrimaryGenerator();

    ///
    void ConfigurePrimaryGenerator();

    ///
    static G4RotationMatrix* m_rotation_matrix;

    ///
    static G4double m_source_isocentre_distance;

    ///
    int m_min_p_vrtx_vec_size = 1;
};

#endif // PRIMARYGENERATIONACTION_H
