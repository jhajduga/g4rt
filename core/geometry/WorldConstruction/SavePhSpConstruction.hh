/**
*
* \author B.Rachwal (brachwal [at] agh.edu.pl)
* \date 10.04.2020
*
*/

#ifndef Dose3D_SAVEPHSPCONSTRUCTION_HH
#define Dose3D_SAVEPHSPCONSTRUCTION_HH

#include "IPhysicalVolume.hh"

///\class SavePhSpConstruction
///\brief The world volume of phase space planes construction.
class SavePhSpConstruction : public IPhysicalVolume {
  public:
  ///
  static SavePhSpConstruction *GetInstance();

  ///
  void Construct(G4VPhysicalVolume *parentPV) override;

  ///
  void Destroy() override;

  ///
  void Reset() override {};

  ///
  G4bool Update() override;

  ///
  void WriteInfo() override;

  ///
  void DefineSensitiveDetector();

  private:
  /**
 * @brief Constructs the SavePhSpConstruction object and initializes the base class with the name "WorldConstruction".
 *
 * This constructor is private and used internally to enforce the singleton pattern.
 */
  SavePhSpConstruction():IPhysicalVolume("WorldConstruction"){}

  /**
 * @brief Default destructor for SavePhSpConstruction.
 */
  ~SavePhSpConstruction() = default;

  /**
 * @brief Copy constructor is deleted to enforce the singleton pattern.
 */
  SavePhSpConstruction(const SavePhSpConstruction &) = delete;

  /**
 * @brief Deleted copy assignment operator to enforce singleton pattern.
 */
SavePhSpConstruction &operator=(const SavePhSpConstruction &) = delete;

  /**
 * @brief Move constructor is deleted to enforce singleton pattern.
 */
SavePhSpConstruction(SavePhSpConstruction &&) = delete;

  /**
 * @brief Deleted move assignment operator to enforce singleton semantics.
 *
 * Prevents moving assignment of SavePhSpConstruction instances.
 */
SavePhSpConstruction &operator=(SavePhSpConstruction &&) = delete;

  ///
  bool ModelSetup();

  ///
  G4VPhysicalVolume *PVPhmWorld;

  ///
  IPhysicalVolume* m_savePhSp;
};

#endif // Dose3D_SAVEPHSPCONSTRUCTION_HH
