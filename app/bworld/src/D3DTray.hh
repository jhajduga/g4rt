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

    ///
    ~D3DTray() {};

    ///
    void Construct(G4VPhysicalVolume *parentPV) override;

    ///
    void Destroy() override {}

    ///
    G4bool Update() override { return true;}

    ///
    void Reset() override {}

    ///
    void WriteInfo() override {}
    
    ///
    void DefineSensitiveDetector();


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