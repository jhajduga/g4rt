/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 10.12.2017
*
*/

#ifndef Dose3D_PHANTOMCONSTRUCTION_HH
#define Dose3D_PHANTOMCONSTRUCTION_HH

#include "G4GeometryManager.hh"
#include "G4RunManager.hh"
#include "G4UImanager.hh"
#include "G4UImessenger.hh"
#include "G4LogicalVolume.hh"
#include "G4VPhysicalVolume.hh"
#include "globals.hh"
#include "IPhysicalVolume.hh"
#include <G4SubtractionSolid.hh>
#include <G4UnionSolid.hh>


class VPatient;

///\class PatientGeometry
///\brief The liniac Phantom volume construction.
class PatientGeometry : public IPhysicalVolume,
                            public Configurable {
  public:
  ///
  static PatientGeometry *GetInstance();
                    
  ///
  void Construct(G4VPhysicalVolume *parentPV) override;

  ///
  void Destroy() override;

  ///
  G4bool Update() override;

  /**
 * @brief No-op reset of the patient geometry; does not modify internal state.
 *
 * This implementation intentionally performs no action.
 */
  void Reset() override {}

  ///
  void WriteInfo() override;

  ///
  void DefineSensitiveDetector();

  ///
  void DefaultConfig(const std::string &unit) override;

  /**
 * @brief Return the current patient model attached to this geometry.
 *
 * Returns the pointer to the VPatient used by this PatientGeometry instance.
 * May be nullptr if no patient has been configured. The caller does not take ownership.
 *
 * @return VPatient* Pointer to the patient model (non-owning), or nullptr if unset.
 */
  VPatient* GetPatient() const { return m_patient; }

  ///
  void ExportToCsvCT(const std::string& path_to_output_dir) const;
  void ExportDoseToCsvCT(const G4Run* runPtr) const;

  private:
  ///
  PatientGeometry();

  ///
  ~PatientGeometry();

  /**
 * @brief Copy constructor is deleted to prevent copying of the singleton instance.
 */
  PatientGeometry(const PatientGeometry &) = delete;

  /**
 * @brief Deleted copy assignment operator to prevent copying of PatientGeometry instances.
 */
PatientGeometry &operator=(const PatientGeometry &) = delete;

  /**
 * @brief Move constructor is deleted to prevent moving of PatientGeometry instances.
 */
PatientGeometry(PatientGeometry &&) = delete;

  /**
 * @brief Deleted move assignment operator to prevent moving of PatientGeometry.
 *
 * The class is a singleton; allowing move assignment would break its unique-instance
 * invariants and transfer internal ownership, so move assignment is explicitly deleted.
 */
PatientGeometry &operator=(PatientGeometry &&) = delete;

  ///
  bool design();

  ///
  void Configure() override;

  ///
  VPatient* m_patient;

  ///
  G4RotationMatrix* m_rotation;

  ///
  struct pair_hash {
      template <class T1, class T2>
      /**
       * @brief Hash functor for std::pair.
       *
       * Produces a combined hash of the pair by XOR-ing the individual hashes of
       * the first and second elements. Suitable for use as a hash function in
       * unordered containers keyed by std::pair.
       *
       * @tparam T1 Type of the first element.
       * @tparam T2 Type of the second element.
       * @param pair The pair to hash.
       * @return std::size_t Combined hash value.
       */
      std::size_t operator() (const std::pair<T1, T2> &pair) const {
          return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
      }
  };

  ///
  G4PVPlacement* m_suplementary_volume = nullptr;
};

#endif // Dose3D_PHANTOMCONSTRUCTION_HH