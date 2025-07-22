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
  ///
  SavePhSpConstruction():IPhysicalVolume("WorldConstruction"){}

  ///
  ~SavePhSpConstruction() = default;

  /// Delete the copy and move constructors
  SavePhSpConstruction(const SavePhSpConstruction &) = delete;

  SavePhSpConstruction &operator=(const SavePhSpConstruction &) = delete;

  SavePhSpConstruction(SavePhSpConstruction &&) = delete;

  SavePhSpConstruction &operator=(SavePhSpConstruction &&) = delete;

  ///
  bool ModelSetup();

  ///
  G4VPhysicalVolume *PVPhmWorld;

  ///
  IPhysicalVolume* m_savePhSp;
};

#endif // Dose3D_SAVEPHSPCONSTRUCTION_HH
