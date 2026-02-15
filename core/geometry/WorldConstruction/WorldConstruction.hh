/**
*
* \author B.Rachwal (brachwal [at] agh.edu.pl)
* \date 14.11.2017
*
*/

#ifndef Dose3D_WORLDCONSTRUCTION_HH
#define Dose3D_WORLDCONSTRUCTION_HH

#include "G4VUserDetectorConstruction.hh"
#include "IPhysicalVolume.hh"
#include "Configurable.hh"

class G4Run;
class PatientGeometry;
class SavePhSpConstruction;
class LinacGeometry;
class BeamMonitoring;

///\class WorldConstruction
///\brief The top level world volume construction factory. Geant4 app manner.
class WorldConstruction : public G4VUserDetectorConstruction,
                          public IPhysicalVolume,
                          public Configurable{
  public:
  ///
  void Destroy() override;

  ///
  G4bool Update() override;
  G4bool Update(int runId);

  /**
 * @brief Resets the world construction state.
 *
 * This implementation is intentionally left empty.
 */
  void Reset() override {}

  ///
  void DefaultConfig(const std::string &unit) override;

  ///
  static WorldConstruction *GetInstance();

  ///
  void ConstructSDandField() override;

  ///
  bool newGeometry();

  ///
  std::string ExportToGDML(const std::string& path, const std::string& fileName, const std::string &worldName = std::string());

  ///
  void checkVolumeOverlap();

  ///
  void WriteInfo() override;

  /**
 * @brief Access the patient (phantom) geometry environment.
 *
 * Returns the pointer to the PatientGeometry instance managed by this WorldConstruction.
 * The pointer may be nullptr if the patient environment has not been created.
 *
 * @return PatientGeometry* Pointer to the patient geometry or nullptr.
 */
  PatientGeometry* PatientEnvironment() { return m_phantomEnv; }
  /**
 * @brief Access the linac (gantry) geometry managed by WorldConstruction.
 *
 * @return LinacGeometry* Pointer to the LinacGeometry instance, or `nullptr` if the linac environment has not been created.
 */
LinacGeometry* LinacEnvironment() { return m_gantryEnv; }


  /// so that the unique_ptr may delete the singleton
  friend std::unique_ptr<WorldConstruction>::deleter_type;

  /// Classes to which the ConstructSD is delegated
  friend class SavePhSpConstruction;
  friend class VPatient;
  friend class BeamMonitoring;

  virtual bool Create();

  ///
  void Configure() override;

  /**
   * @brief Return custom patient detectors used by the world.
   *
   * By default returns an empty vector. Override to supply any custom
   * VPatient detectors that should be integrated into the simulation.
   *
   * @return std::vector<VPatient*> A vector of non-owning pointers to custom detectors.
   *         Returned pointers must remain valid for the lifetime of the world construction.
   */
  virtual std::vector<VPatient*> GetCustomDetectors() const {
    return std::vector<VPatient*>();
  }
  
  protected:
  ///
  WorldConstruction();

  ///
  ~WorldConstruction();

  /// have to implement pure virtual function
  G4VPhysicalVolume* Construct() override;      // <- G4VUserDetectorConstruction

  ///
  bool ConstructWorldModules(G4VPhysicalVolume *parentPV);
  
  private:
  /**
 * @brief Deleted copy constructor to enforce singleton behavior.
 */
  WorldConstruction(const WorldConstruction &) = delete;

  /**
 * @brief Deleted copy assignment operator to enforce singleton behavior.
 *
 * Prevents copying of the WorldConstruction instance.
 */
WorldConstruction &operator=(const WorldConstruction &) = delete;

  /**
 * @brief Move constructor is deleted to enforce singleton behavior.
 */
WorldConstruction(WorldConstruction &&) = delete;

  /**
 * @brief Deleted move assignment operator to enforce singleton behavior.
 *
 * Prevents moving assignment of WorldConstruction instances.
 */
WorldConstruction &operator=(WorldConstruction &&) = delete;


  /**
 * @brief Empty implementation of the pure virtual Construct method from IPhysicalVolume.
 *
 * This method is required to satisfy the interface but is intentionally left unimplemented in this class.
 */
  void Construct(G4VPhysicalVolume*) override {}  // <- IPhysicalVolume
  /**
 * @brief Access the world physical volume.
 *
 * @return G4VPhysicalVolume* Pointer to the constructed world physical volume, or `nullptr` if it has not been created.
 */

  G4VPhysicalVolume* GetWorldPV() { return m_worldPV; }

  ///
  PatientGeometry* m_phantomEnv = nullptr;

  ///
  SavePhSpConstruction* m_savePhSpEnv = nullptr;

  ///
  LinacGeometry* m_gantryEnv = nullptr;

  ///
  BeamMonitoring* m_beamMonitoring = nullptr;

  ///
  G4VPhysicalVolume* m_worldPV = nullptr;
};

#endif  // Dose3D_WORLDCONSTRUCTION_HH