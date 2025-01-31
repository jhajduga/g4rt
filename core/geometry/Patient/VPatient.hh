//
// Created by brachwal on 28.04.2020.
//

#ifndef DOSE3DVPATIENT_HH
#define DOSE3DVPATIENT_HH

#include "G4Cache.hh"
#include "IPhysicalVolume.hh"
#include "TomlConfigModule.hh"
// #include "Logable.hh"
#include "VoxelHit.hh"
#include <map>

class VPatientSD;

class VPatient : public IPhysicalVolume, public TomlConfigModule{
// class VPatient : public IPhysicalVolume, public TomlConfigModule, public Logable {
  protected:
    ///
    bool m_tracks_analysis = false;

    ///
    G4ThreeVector m_patient_top_position_in_world_env;

    ///
    void SetSensitiveDetector(const G4String& logicalVName, VPatientSD* sensitiveDetectorPtr);

    ///
    virtual void ConstructSensitiveDetector(){};

    /// Pointer to the sensitive detectors, wrapped into the MT service
    G4Cache<VPatientSD*> m_patientSD;

  public:
    ///
    VPatient() = delete;
    
    ///
    // explicit VPatient(const std::string& name):IPhysicalVolume(name),TomlConfigModule(name),Logable("GeoAndScoring"){
    explicit VPatient(const std::string& name):IPhysicalVolume(name),TomlConfigModule(name){
      m_patientSD.Put(nullptr);
    }

    ///
    ~VPatient() = default;

    ///
    void SetTracksAnalysis(bool flag) {m_tracks_analysis = flag; }

    ///
    virtual G4bool IsInside(double x, double y, double z) { return false; }

    ///
    G4ThreeVector GetPatientTopPositionInWolrdEnv() {return m_patient_top_position_in_world_env; }

    /// TO BE DELETED
    virtual std::map<std::size_t, VoxelHit> GetScoringHashedMap(const std::string& name, bool voxelised) const {
      return std::map<std::size_t, VoxelHit>();
    }
    VPatientSD* GetSD() const { return m_patientSD.Get(); }

    virtual std::map<std::size_t, VoxelHit> GetScoringHashedMap(const G4String&,Scoring::Type) const {
      // LOGSVC_WARN("Returning empty scoring hashed map!");
      return std::map<std::size_t, VoxelHit>();
    }


};
#endif //Dose3DVPatient_HH
