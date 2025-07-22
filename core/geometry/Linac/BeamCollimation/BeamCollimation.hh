/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 18.05.2020
*
*/

#ifndef Dose3D_VARIAN_TRUEBEAM_HEAD_MOCKUP_HH
#define Dose3D_VARIAN_TRUEBEAM_HEAD_MOCKUP_HH

#include "IPhysicalVolume.hh"
#include "G4PrimaryVertex.hh"
#include "Types.hh"
#include "VMlc.hh"
#include <unordered_map>

class G4VPhysicalVolume;

///\class BeamCollimation
class BeamCollimation : public IPhysicalVolume, public RunComponet {
  public:
  ///
  static BeamCollimation *GetInstance();

  ///
  void Construct(G4VPhysicalVolume *parentPV) override;

  ///
  void Destroy() override;

  /**
   * @brief Placeholder for updating the beam collimation state.
   *
   * Currently unimplemented; always returns true.
   *
   * @return G4bool Always returns true.
   */
  G4bool Update() override {
    G4cout << "BeamCollimation::Update::Implement me." << G4endl;
    return true;
  }

  ///
  void Reset() override;

  ///
  void WriteInfo() override;

  static void FilterPrimaries(std::vector<G4PrimaryVertex*>& p_vrtx);

  static G4ThreeVector SetParticlePositionBeforeCollimators(G4PrimaryVertex* vrtx, G4double finalZ);

  /**
 * @brief Returns the pointer to the multi-leaf collimator (MLC) object.
 *
 * @return VMlc* Pointer to the MLC instance managed by the collimation system.
 */
VMlc* GetMlc() const { return m_mlc; }

  static G4double AfterMLC;
  static G4double BeforeMLC;
  static G4double BeforeJaws;
  static G4double ParticleAngleTreshold;
  /**
 * @brief Returns a pointer to the multi-leaf collimator (MLC) object.
 *
 * @return VMlc* Pointer to the MLC instance managed by the collimation system.
 */
VMlc* GetMlc() { return m_mlc; }

  void SetRunConfiguration(const ControlPoint* ) override;


  private:
  ///
  BeamCollimation();

  ///
  ~BeamCollimation();

  /**
 * @brief Copy constructor is deleted to enforce singleton pattern.
 */
  BeamCollimation(const BeamCollimation &) = delete;

  /**
 * @brief Deleted copy assignment operator to enforce singleton pattern.
 *
 * Prevents copying of BeamCollimation instances.
 */
BeamCollimation &operator=(const BeamCollimation &) = delete;

  /**
 * @brief Move constructor is deleted to enforce singleton pattern.
 */
BeamCollimation(BeamCollimation &&) = delete;

  /**
 * @brief Deleted move assignment operator to enforce singleton pattern.
 *
 * Prevents moving assignment of BeamCollimation instances to ensure only one instance exists.
 */
BeamCollimation &operator=(BeamCollimation &&) = delete;

  ///
  std::map<G4String, G4VPhysicalVolume *> m_physicalVolume;

  ///
  void AcceptRunVisitor(RunSvc *visitor) override;

  ///
  void SetJawAperture(const std::string& name, G4ThreeVector &centre, G4ThreeVector halfSize, G4RotationMatrix *cRotation);

  ///
  bool Jaws(G4VPhysicalVolume *parentWorld);

  /// 
  bool MLC(G4VPhysicalVolume *parentWorld);

  ///
  static VMlc* m_mlc;

  ///
  std::unordered_map<std::string, double> m_apertures;

  /**
 * @brief Placeholder for defining sensitive detectors in the collimation system.
 *
 * This method is currently unimplemented.
 */
  void DefineSensitiveDetector() {}

};

#endif // Dose3D_VARIAN_TRUEBEAM_HEAD_MOCKUP_HH
