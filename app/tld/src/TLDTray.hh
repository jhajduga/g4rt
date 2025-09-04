#ifndef Dose3D_TRAYCONSTRUCTION_HH
#define Dose3D_TRAYCONSTRUCTION_HH

#include "IPhysicalVolume.hh"
#include "TomlConfigurable.hh"
#include "Services.hh"
#include "TLD.hh"

///
// class TLDTray : public IPhysicalVolume, public TomlConfigModule {
class TLDTray : public VPatient {
    private:
        ///
        void ParseTomlConfig() override;

        ///
        void LoadConfiguration();

        class Config {
            public:
                std::string m_tld_medium = "None";

                G4int m_nX_tld = 3;
                G4int m_nY_tld = 4;
                G4int m_nZ_tld = 1;
                
                G4ThreeVector m_top_position_in_env;
                
                G4int m_tld_nX_voxels = 10;
                G4int m_tld_nY_voxels = 10;
                G4int m_tld_nZ_voxels = 10;

                G4String m_stl_geometry_file_path = "None";
                
                bool m_initialized = false;
            };

    public:
    ///
    TLDTray(G4VPhysicalVolume *parentPV, const std::string& name);

    /**
 * @brief Default destructor for TLDTray.
 *
 * No special cleanup is performed.
 */
    ~TLDTray() {};

    ///
    void Construct(G4VPhysicalVolume *parentPV) override;

    /**
 * @brief Placeholder for tray destruction logic.
 *
 * This method is intentionally left empty as no explicit destruction steps are required for the TLD tray.
 */
    void Destroy() override {}

    /**
 * @brief Confirms that the TLD tray state is up to date.
 *
 * @return Always returns true.
 */
    G4bool Update() override { return true;}

    /**
 * @brief Resets the TLD tray state.
 *
 * This override currently performs no action.
 */
    void Reset() override {}

    /**
 * @brief Placeholder for writing tray information; currently does nothing.
 */
    void WriteInfo() override {}
    
    ///
    void DefineSensitiveDetector();


    // VPatient* GetDetector() const { return m_detector; }

    G4ThreeVector m_global_centre;
    G4ThreeVector m_tray_world_halfSize;
    std::string m_tray_name;
    std::string m_tray_config_file;
    G4RotationMatrix m_rot;

    ///
    std::vector<TLD*> m_tld_detectors;

    ///
    TLDTray::Config m_config;

    ///
    std::map<std::size_t, VoxelHit> GetScoringHashedMap(const G4String& scoring_name,Scoring::Type type) const override;

};


#endif //Dose3D_TRAYCONSTRUCTION_HH