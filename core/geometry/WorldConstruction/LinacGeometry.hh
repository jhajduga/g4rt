/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 14.11.2017
*
*/

#ifndef Dose3D_HEADCONSTUCTION_HH
#define Dose3D_HEADCONSTUCTION_HH

#include "IPhysicalVolume.hh"
#include "Configurable.hh"

class G4Run;
class G4VPhysicalVolume;
class VMlc;

///\class LinacGeometry
///\brief The linac head level volume construction factory.
class LinacGeometry : public Configurable, public IPhysicalVolume {
  public:
  static LinacGeometry *GetInstance();

  void Construct(G4VPhysicalVolume *parentPV) override;

  void Destroy() override;

  G4bool Update() override;

  void Reset() override {};

  void ResetHead();

  void WriteInfo() override;

  // TODO: No geometry roration - but particle should be rotate after going through Jaws and MLC
  // G4RotationMatrix *rotateHead();

  // G4RotationMatrix *rotateHead(G4double angleX);

  ///
  void DefineSensitiveDetector();


  static void SetIsocentreDistance(double distance_cm);
  static double GetIsocentreDistance();

  private:
  LinacGeometry();

  /**
 * @brief Destroys the LinacGeometry instance.
 *
 * Cleans up resources associated with the LinacGeometry object upon destruction.
 */
~LinacGeometry() = default;

  /**
 * @brief Deleted copy constructor to prevent copying of LinacGeometry instances.
 */
  LinacGeometry(const LinacGeometry &) = delete;

  /**
 * @brief Deleted copy assignment operator to prevent copying of the singleton LinacGeometry.
 *
 * Copy assignment is explicitly deleted to enforce singleton semantics and avoid duplicating internal state.
 */
LinacGeometry &operator=(const LinacGeometry &) = delete;

  /**
 * @brief Move constructor is deleted to prevent moving of the singleton instance.
 */
LinacGeometry(LinacGeometry &&) = delete;

  /**
 * @brief Deleted move assignment operator to prevent moving of LinacGeometry instances.
 */
LinacGeometry &operator=(LinacGeometry &&) = delete;

  bool design();

  void DefaultConfig(const std::string &unit) override;

  void Configure() override;

  /// Linac Head:

  ///
  IPhysicalVolume* m_headInstance = nullptr;

  /// Periferials:

  ///
  IPhysicalVolume* m_tableInstance = nullptr;

  ///
  IPhysicalVolume* m_obiInstance = nullptr;

  ///
  IPhysicalVolume* m_epidInstance = nullptr;


  static inline double s_isocentre_distance_cm = 0.0;
};

#endif  // Dose3D_HEADCONSTUCTION_HH
