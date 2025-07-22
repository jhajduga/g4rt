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

  ///
  void Reset() override {}

  ///
  void WriteInfo() override;

  ///
  void DefineSensitiveDetector();

  ///
  void DefaultConfig(const std::string &unit) override;

  ///
  VPatient* GetPatient() const { return m_patient; }

  ///
  void ExportToCsvCT(const std::string& path_to_output_dir) const;
  void ExportDoseToCsvCT(const G4Run* runPtr) const;

  private:
  ///
  PatientGeometry();

  ///
  ~PatientGeometry();

  /// Delete the copy and move constructors
  PatientGeometry(const PatientGeometry &) = delete;

  PatientGeometry &operator=(const PatientGeometry &) = delete;

  PatientGeometry(PatientGeometry &&) = delete;

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
      std::size_t operator() (const std::pair<T1, T2> &pair) const {
          return std::hash<T1>()(pair.first) ^ std::hash<T2>()(pair.second);
      }
  };

  ///
  G4PVPlacement* m_suplementary_volume = nullptr;
};

#endif // Dose3D_PHANTOMCONSTRUCTION_HH
