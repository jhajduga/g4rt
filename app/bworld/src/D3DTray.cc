
#include "D3DTray.hh"
#include "Services.hh"
#include "G4Box.hh"

////////////////////////////////////////////////////////////////////////////////
///
D3DTray::D3DTray(G4VPhysicalVolume *parentPV, const std::string& name)
:IPhysicalVolume(name), TomlConfigModule(name), m_tray_name(name) {
    m_detector = new D3DDetector(m_tray_name);
    LoadConfiguration();
    Construct(parentPV);
} 

////////////////////////////////////////////////////////////////////////////////
///
void D3DTray::Construct(G4VPhysicalVolume *parentPV) {
    auto medium = Service<ConfigSvc>()->GetValue<G4MaterialSPtr>("MaterialsSvc", "Usr_G4AIR20C");
    
    auto boxName = m_tray_name + "EnvBox";
    G4Box *patientEnv = new G4Box(m_tray_name, m_tray_world_halfSize.x(), m_tray_world_halfSize.y(), m_tray_world_halfSize.z());
    
    auto LVName = m_tray_name + "EnvLV";
    G4LogicalVolume *patientEnvLV = new G4LogicalVolume(patientEnv, medium.get(), LVName, 0, 0, 0);
    
    auto PVName = m_tray_name + "EnvPV";
    auto pv = new G4PVPlacement(&m_rot, m_global_centre, PVName, patientEnvLV, parentPV, false, 0);


    m_detector->Construct(pv);
    m_detector->WriteInfo();
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DTray::DefineSensitiveDetector() {
    m_detector->DefineSensitiveDetector();
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DTray::LoadConfiguration(){

    // Deafult configuration
    m_rot = G4RotationMatrix(); //.rotateY(180.*deg);
    m_tray_world_halfSize = G4ThreeVector(102.,111.,9.2);
    
    m_global_centre = G4ThreeVector(0.0,0.0,0.0);

    m_det_config.m_cell_nX_voxels = 4;
    m_det_config.m_cell_nY_voxels = 4;
    m_det_config.m_cell_nZ_voxels = 4;

    m_det_config.m_cell_medium = "RMPS470";
    m_det_config.m_stl_positioning_file_path = "dose3d/geo/Tray/4x5x1_tray.csv";
    m_det_config.m_stl_geometry_file_path = "dose3d/geo/Tray/tray.stl";


    // Any config value is being replaced by the one existing in TOML config 
    ParseTomlConfig();

    // Finally, inject the configuration to the detector instance
    dynamic_cast<D3DDetector*>(m_detector)->SetConfig(m_det_config);
}

////////////////////////////////////////////////////////////////////////////////
///
void D3DTray::ParseTomlConfig(){
    SetTomlConfigFile(); // it set the job main file for searching this configuration
    auto configFile = GetTomlConfigFile();
    if (!svc::checkIfFileExist(configFile)) {
        LOGSVC_CRITICAL("D3DTray::TConfigurarable::ParseTomlConfig::File {} not fount.", configFile);
        exit(1);
    }
    auto config = toml::parse_file(configFile);
    auto configPrefix = GetTomlConfigPrefix();
    LOGSVC_INFO("D3DTray::Importing configuration from: {}:{}",configFile,configPrefix);

    m_global_centre.setX(config[configPrefix]["Position"][0].value_or(0.0));
    m_global_centre.setY(config[configPrefix]["Position"][1].value_or(0.0));
    m_global_centre.setZ(config[configPrefix]["Position"][2].value_or(0.0));
    // G4cout << "global_centre: " << m_global_centre << G4endl;

    auto vox_nX = config[configPrefix]["CellVoxelization"][0].value_or(0);
    if(vox_nX > 0 ) 
        m_det_config.m_cell_nX_voxels = vox_nX;
    auto vox_nY = config[configPrefix]["CellVoxelization"][1].value_or(0);
    if(vox_nY > 0 ) 
        m_det_config.m_cell_nY_voxels = vox_nY;
    auto vox_nZ = config[configPrefix]["CellVoxelization"][2].value_or(0);
    if(vox_nZ > 0 ) 
        m_det_config.m_cell_nZ_voxels = vox_nZ;

    auto rotX = config[configPrefix]["Rotation"][0].value_or(0);
    if(rotX > 0 ) 
        m_rot.rotateX(rotX*deg);
    auto rotY = config[configPrefix]["Rotation"][1].value_or(0);
    if(rotY > 0 ) 
        m_rot.rotateY(rotY*deg);
    auto rotZ = config[configPrefix]["Rotation"][2].value_or(0);
    if(rotZ > 0 )           
        m_rot.rotateZ(rotZ*deg);


}

