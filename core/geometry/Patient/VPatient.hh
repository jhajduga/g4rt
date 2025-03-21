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
    /**
 * @brief Deleted default constructor.
 *
 * VPatient objects must be constructed with a name; this default constructor is deleted to prevent uninitialized instances.
 */
    VPatient() = delete;
    
    ///
    /**
     * @brief Constructs a new VPatient object with the given name.
     *
     * Initializes the base classes IPhysicalVolume and TomlConfigModule using the provided name.
     * The internal sensitive detector cache is initialized to null.
     *
     * @param name A unique identifier for the patient volume.
     */
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
    /**
 * @brief Retrieves the cached sensitive detector for the patient.
 *
 * This function returns the pointer to the sensitive detector stored in the patient's detector cache.
 *
 * @return VPatientSD* Pointer to the cached sensitive detector, or nullptr if it has not been set.
 */
VPatientSD* GetSD() const { return m_patientSD.Get(); }

    /**
     * @brief Retrieves an empty scoring hashed map.
     *
     * This virtual method serves as a stub implementation and always returns an empty map of voxel hits.
     * The provided parameters are ignored and exist solely to maintain interface consistency for derived classes.
     *
     * @param scoringId Unused parameter.
     * @param scoringType Unused parameter.
     * @return An empty map of voxel hits.
     */
    virtual std::map<std::size_t, VoxelHit> GetScoringHashedMap(const G4String&,Scoring::Type) const {
      // LOGSVC_WARN("Returning empty scoring hashed map!");
      return std::map<std::size_t, VoxelHit>();
    }


};
#endif //Dose3DVPatient_HH
