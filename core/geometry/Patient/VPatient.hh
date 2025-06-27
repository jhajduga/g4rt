//
// Created by brachwal on 28.04.2020.
//

#ifndef DOSE3DVPATIENT_HH
#define DOSE3DVPATIENT_HH

#include "G4Cache.hh"
#include "IPhysicalVolume.hh"
#include "TomlConfigModule.hh"
#include "Logable.hh"
#include "VoxelHit.hh"
#include <map>

class VPatientSD;

class VPatient : public IPhysicalVolume, public TomlConfigModule, public Logable {
  protected:
    ///

   ///
   void SetSensitiveDetector(const G4String& logicalVName, VPatientSD* sensitiveDetectorPtr);

   ///
   virtual void ConstructSensitiveDetector() {};

   /// Pointer to the sensitive detectors, wrapped into the MT service
   G4Cache<VPatientSD*> m_patientSD;

   /// Volume in mm^3
   /// Initial value should be overritten by the final class,
   /// otherwise calling the GetVolume method will throw an exception.
   G4double m_volume = -1.;

  public:
    ///
    VPatient() = delete;
    
    ///
    explicit VPatient(const std::string& name):IPhysicalVolume(name),TomlConfigModule(name),Logable("GeoAndScoring"){
      m_patientSD.Put(nullptr);
    }

    ///
    ~VPatient() = default;

    ///

    ///
    virtual G4bool IsInside(double x, double y, double z) { return false; }

    /// TO BE DELETED
    virtual std::map<std::size_t, VoxelHit> GetScoringHashedMap(const std::string& name, bool voxelised) const {
      return std::map<std::size_t, VoxelHit>();
    }
    VPatientSD* GetSD() const { return m_patientSD.Get(); }

    virtual std::map<std::size_t, VoxelHit> GetScoringHashedMap(const G4String&,Scoring::Type) const {
      LOGSVC_WARN("Returning empty scoring hashed map!");
      return std::map<std::size_t, VoxelHit>();
    }

    ///
    void SetVolume(G4double volume) {m_volume = volume; };

    ///
    G4double GetVolume() const;

    ///
    virtual G4double GetCellVolume() const { return GetVolume(); };


};
#endif //Dose3DVPatient_HH
