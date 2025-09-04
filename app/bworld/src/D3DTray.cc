#include "D3DTray.hh"
#include "Services.hh"
#include "G4Box.hh"

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs a D3DTray instance, initializing geometry and configuration.
 *
 * Initializes the tray with the specified parent physical volume and name, sets up the associated detector, loads configuration parameters (including overrides from a TOML file), and constructs the tray's geometry within the simulation.
 *
 * @param parentPV The parent Geant4 physical volume to which this tray will be attached.
 * @param name The unique name for this tray instance.
 */
D3DTray::D3DTray(G4VPhysicalVolume *parentPV, const std::string& name)
:IPhysicalVolume(name), TomlConfigModule(name), m_tray_name(name) {
    m_detector = new D3DDetector(m_tray_name);
    LoadConfiguration();
    Construct(parentPV);
} 

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs the 3D tray environment and places it within the parent physical volume.
 *
 * Creates a box-shaped Geant4 volume representing the tray environment, assigns it a material, and places it in the simulation geometry with the configured rotation and position. Delegates further construction and information output to the associated detector instance.
 *
 * @param parentPV The parent Geant4 physical volume into which the tray environment is placed.
 */
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
/**
 * @brief Delegates the definition of sensitive detectors to the associated detector.
 *
 * Calls the `DefineSensitiveDetector()` method of the contained `D3DDetector` instance to set up sensitive detector components for the tray.
 */
void D3DTray::DefineSensitiveDetector() {
    m_detector->DefineSensitiveDetector();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Load default geometry and detector settings, apply TOML overrides, and pass the final configuration to the detector.
 *
 * Loads a set of sensible defaults for the tray (rotation, world half-size, global center) and detector parameters
 * (voxel counts, medium, STL file paths), then calls ParseTomlConfig() to override any values present in the TOML
 * configuration. The finalized configuration is injected into the contained D3DDetector via SetConfig().
 *
 * Side effects:
 * - May read and parse a TOML file; ParseTomlConfig() will log a fatal error and terminate the program if a required
 *   TOML configuration file is missing.
 * - Updates member state: m_rot, m_tray_world_halfSize, m_global_centre, and m_det_config.
 */
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
/**
 * @brief Parses and applies tray configuration parameters from a TOML file.
 *
 * Reads the specified TOML configuration file, extracts position, voxelization, and rotation parameters under the configured prefix, and updates the tray's global center, detector voxelization, and rotation accordingly. If the configuration file does not exist, logs a fatal error and terminates the program.
 */
void D3DTray::ParseTomlConfig(){
    SetTomlConfigFile(); // it set the job main file for searching this configuration
    auto configFile = GetTomlConfigFile();
    if (!svc::checkIfFileExist(configFile)) {
        FATAL_GEO("D3DTray::TConfigurarable::ParseTomlConfig::File {} not fount.", configFile);
        exit(1);
    }
    auto config = toml::parse_file(configFile);
    auto configPrefix = GetTomlConfigPrefix();
    INFO_GEO("D3DTray::Importing configuration from: {}:{}",configFile,configPrefix);

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

