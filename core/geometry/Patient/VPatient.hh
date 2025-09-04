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
     * @brief Create a VPatient with the given name.
     *
     * Initializes IPhysicalVolume and TomlConfigModule with the provided name and sets the internal
     * sensitive-detector cache to nullptr.
     *
     * @param name Logical name of this patient volume.
     */
    explicit VPatient(const std::string& name):IPhysicalVolume(name),TomlConfigModule(name){
      m_patientSD.Put(nullptr);
    }

    /**
 * @brief Destroys the VPatient object.
 *
 * Default destructor; performs no additional cleanup.
 */
    ~VPatient() = default;

    ///

    /**
 * @brief Determines whether a point is inside the patient volume.
 *
 * Always returns false in the base class. Derived classes should override this method to implement geometry-specific containment logic.
 *
 * @param x X-coordinate of the point.
 * @param y Y-coordinate of the point.
 * @param z Z-coordinate of the point.
 * @return G4bool True if the point is inside the volume; false otherwise.
 */
    virtual G4bool IsInside(double x, double y, double z) { return false; }

    /**
     * @brief Returns an empty scoring map for the specified name and voxelization flag.
     *
     * This method is a placeholder and is marked for deletion. It always returns an empty map.
     *
     * @return std::map<std::size_t, VoxelHit> An empty map.
     */
    virtual std::map<std::size_t, VoxelHit> GetScoringHashedMap(const std::string& name, bool voxelised) const {
      return std::map<std::size_t, VoxelHit>();
    }
    /**
 * @brief Get the associated sensitive detector.
 *
 * Returns the (non-owning) pointer to the patient sensitive detector stored in the per-thread cache.
 * May be nullptr if no sensitive detector has been set.
 *
 * @return VPatientSD* Pointer to the sensitive detector, or nullptr if none is configured.
 */
VPatientSD* GetSD() const { return m_patientSD.Get(); }

    /**
     * @brief Default fallback: return an empty scoring hashed map for the requested scoring type.
     *
     * The base implementation logs a warning and returns an empty std::map. Derived patient volumes
     * should override this to return actual voxel scoring data for the given scoring type.
     *
     * @return std::map<std::size_t, VoxelHit> Empty map when no scoring is available.
     */
    virtual std::map<std::size_t, VoxelHit> GetScoringHashedMap(const G4String&,Scoring::Type) const {
      WARN_GEO("Returning empty scoring hashed map!");
      return std::map<std::size_t, VoxelHit>();
    }

    /**
 * @brief Set the patient's total volume.
 *
 * Records the patient volume (used by GetVolume() and the default
 * GetCellVolume()).
 *
 * @param volume Total volume in cubic millimeters (mm^3).
 */
    void SetVolume(G4double volume) {m_volume = volume; };

    ///
    G4double GetVolume() const;

    /**
 * @brief Returns the volume of a single cell within the patient volume.
 *
 * By default, this returns the total volume. Derived classes can override this method to provide cell-specific volume calculations.
 *
 * @return G4double The cell volume in cubic millimeters.
 */
    virtual G4double GetCellVolume() const { return GetVolume(); };


};
#endif //Dose3DVPatient_HH
