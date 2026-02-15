//
// Created by brachwal on 28.04.2020.
//

#ifndef DOSE3DVPATIENT_HH
#define DOSE3DVPATIENT_HH

#include "G4Cache.hh"
#include "IPhysicalVolume.hh"
#include "TomlConfigModule.hh"
#include "VoxelHit.hh"
#include "LogSvc.hh"
#include <map>

class VPatientSD;

class VPatient : public IPhysicalVolume, public TomlConfigModule{
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
    /**
 * @brief Default constructor is deleted to enforce explicit initialization with a name.
 */
    VPatient() = delete;
    
    /**
     * @brief Construct a VPatient identified by the given logical name.
     *
     * Resets the per-thread sensitive-detector cache to nullptr.
     *
     * @param name Logical name of this patient volume.
     */
    explicit VPatient(const std::string& name):IPhysicalVolume(name),TomlConfigModule(name){
      m_patientSD.Put(nullptr);
    }

    /**
 * @brief Default destructor.
 *
 * Uses compiler-generated cleanup; no additional actions are performed.
 */
    ~VPatient() = default;

    ///

    /**
 * @brief Check whether a point lies inside this patient volume.
 *
 * The base implementation always returns false; derived classes should override to provide geometry-specific containment.
 *
 * @param x X-coordinate of the point.
 * @param y Y-coordinate of the point.
 * @param z Z-coordinate of the point.
 * @return G4bool `true` if the point is inside the volume, `false` otherwise.
 */
    virtual G4bool IsInside(double x, double y, double z) { return false; }

    /**
     * @brief Provide a scoring hashed map for a named scoring region, optionally voxelised.
     *
     * Default implementation provides no scoring data.
     *
     * @param name Identifier of the scoring region.
     * @param voxelised If `true`, request voxelised scoring; otherwise request region-level scoring.
     * @return std::map<std::size_t, VoxelHit> An empty map (no scored voxels).
     */
    virtual std::map<std::size_t, VoxelHit> GetScoringHashedMap(const std::string& name, bool voxelised) const {
      return std::map<std::size_t, VoxelHit>();
    }
    /**
 * @brief Retrieves the non-owning sensitive detector pointer associated with this patient.
 *
 * The pointer is stored per-thread in an internal cache and may be nullptr if no sensitive detector has been set.
 *
 * @return VPatientSD* Pointer to the sensitive detector, or nullptr if none is configured.
 */
VPatientSD* GetSD() const { return m_patientSD.Get(); }

    /**
     * @brief Fallback that returns an empty hashed map for the requested scoring type.
     *
     * Derived patient volumes should override this to provide voxel scoring data for the given scoring type.
     *
     * @return std::map<std::size_t, VoxelHit> Empty map when no scoring data is available for the requested type.
     */
    virtual std::map<std::size_t, VoxelHit> GetScoringHashedMap(const G4String&,Scoring::Type) const {
      WARN_GEO("Returning empty scoring hashed map!");
      return std::map<std::size_t, VoxelHit>();
    }

    /**
 * @brief Store the patient's total volume in cubic millimeters.
 *
 * Records the value used by GetVolume() and, by default, GetCellVolume().
 *
 * @param volume Total volume in cubic millimeters (mm^3).
 */
    void SetVolume(G4double volume) {m_volume = volume; };

    ///
    G4double GetVolume() const;

    /**
 * @brief Provides the volume of a single cell within the patient volume.
 *
 * By default this returns the total volume; derived classes may override to return a cell-specific volume.
 *
 * @return G4double Cell volume in cubic millimeters.
 */
    virtual G4double GetCellVolume() const { return GetVolume(); };


};
#endif //Dose3DVPatient_HH