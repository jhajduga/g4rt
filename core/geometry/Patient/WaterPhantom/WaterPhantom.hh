/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 10.12.2017
*
*/

#ifndef WATER_PHANTOM_HH
#define WATER_PHANTOM_HH

#include "G4PVPlacement.hh"
#include "VPatient.hh"

///\class WaterPhantom
///\brief The Phantom filled with water
class WaterPhantom : public VPatient {
  public:
  /// 
  WaterPhantom();

  /// 
  ~WaterPhantom();

  /// 
  void Construct(G4VPhysicalVolume *parentPV) override;

  /// 
  void Destroy() override;

  /// 
  G4bool Update() override;

  /**
 * @brief Placeholder reset for the WaterPhantom.
 *
 * Restores the phantom to its initial configuration and clears any accumulated
 * runtime or scoring state when implemented. Currently performs no state changes
 * and only logs an informational message.
 */
  void Reset() override { G4cout << "Implement me." << G4endl; }

  ///  
  void WriteInfo() override;

  ///
  void DefineSensitiveDetector() override;

  ///
  std::map<std::size_t, VoxelHit> GetScoringHashedMap(const G4String& scoring_name,Scoring::Type type) const override;

  ///
  friend class PatientTest_WaterPhantomScoring_Test;

  private:

  ///
  G4bool LoadParameterization();

  ///
  G4bool LoadDefaultParameterization();

  ///
  void ParseTomlConfig() override;

  ///
  void ConstructSensitiveDetector() override;

  ///
  void ConstructFullVolumeScoring(const G4String& name);

  ///
  void ConstructFarmerVolumeScoring(const G4String& name);

  ///
  std::string m_phantomMedium = "None";

  /// 
  G4double m_centrePositionX = 0.0;
  G4double m_centrePositionY = 0.0;
  G4double m_centrePositionZ = 0.0;

  /// 
  G4double m_sizeX = 0.0;
  G4double m_sizeY = 0.0;
  G4double m_sizeZ = 0.0;

  ///
  G4int m_detectorVoxelizationX = 0;
  G4int m_detectorVoxelizationY = 0;
  G4int m_detectorVoxelizationZ = 0;

  ///
  G4bool m_watertankScoring = true;

  ///
  G4bool m_farmerScoring = false;

};

#endif  // WATER_PHANTOM_HH