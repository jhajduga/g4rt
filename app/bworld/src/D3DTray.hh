#ifndef Dose3D_TRAYCONSTRUCTION_HH
#define Dose3D_TRAYCONSTRUCTION_HH

#include "IPhysicalVolume.hh"
#include "TomlConfigurable.hh"
#include "Services.hh"
#include "D3DDetector.hh"

///\class PatientGeometry
///\brief The liniac Phantom volume construction.
class D3DTray : public IPhysicalVolume, public TomlConfigModule {
    private:
        ///
        void ParseTomlConfig() override;

        ///
        void LoadConfiguration();

        ///
        D3DDetector::Config m_det_config;

    public:
    ///
    D3DTray(G4VPhysicalVolume *parentPV, const std::string& name);

    /**
 * @brief Destructor for D3DTray.
 *
 * Releases object resources; no special cleanup is required by this class.
 */
    ~D3DTray() {};

    ///
    void Construct(G4VPhysicalVolume *parentPV) override;

    /**
 * @brief Performs no action when destroying the tray volume.
 *
 * This override is intentionally left empty as no cleanup is required for this class.
 */
    void Destroy() override {}

    /**
 * @brief Updates the tray state.
 *
 * Always returns true, indicating the tray is up-to-date.
 *
 * @return G4bool Always true.
 */
    G4bool Update() override { return true;}

    /**
 * @brief Resets the tray state.
 *
 * This implementation performs no action.
 */
    void Reset() override {}

    /**
 * @brief No-op implementation for writing tray information.
 *
 * This method is intentionally left empty and does not perform any action.
 */
    void WriteInfo() override {}
    
    ///
    void DefineSensitiveDetector();


    /**
 * @brief Get the detector volume contained in this tray.
 *
 * @return VPatient* Pointer to the contained detector, or nullptr if no detector is present.
 */
VPatient* GetDetector() const { return m_detector; }

    G4ThreeVector m_global_centre;
    G4ThreeVector m_tray_world_halfSize;
    std::string m_tray_name;
    std::string m_tray_config_file;
    G4RotationMatrix m_rot;

    ///
    VPatient* m_detector;

};


#endif //Dose3D_TRAYCONSTRUCTION_HH