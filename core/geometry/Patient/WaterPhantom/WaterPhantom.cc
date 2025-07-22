#include "WaterPhantom.hh"
#include "G4SystemOfUnits.hh"
#include "G4Box.hh"
#include "G4ProductionCuts.hh"
#include "WaterPhantomSD.hh"
#include "NTupleEventAnalisys.hh"
#include "G4UserLimits.hh"
#include "toml.hh"
#include "Services.hh"
#include "D3DCell.hh"

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs a WaterPhantom object with the default name.
 *
 * Initializes the WaterPhantom as a subclass of VPatient, setting its name to "WaterPhantom".
 */
WaterPhantom::WaterPhantom() : VPatient("WaterPhantom") {}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Destructor for the WaterPhantom class.
 *
 * Cleans up resources by destroying the phantom's physical volume if it exists.
 */
WaterPhantom::~WaterPhantom() { Destroy(); }

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Parses and loads water phantom configuration parameters from a TOML file.
 *
 * Reads geometry, material, and scoring settings for the water phantom from a TOML configuration file using a required prefix. Validates the existence of the configuration file and prefix, throwing a Geant4 exception if either is missing. Extracts phantom size, center position, voxelization, medium type, and scoring flags from the configuration.
 */
void WaterPhantom::ParseTomlConfig() {
  auto configFile = GetTomlConfigFile();
  auto configPrefix = GetTomlConfigPrefix();
  INFO_GEO("Importing configuration from:\n{}", configFile);

  if (!svc::checkIfFileExist(configFile)) {
    FATAL_GEO("File {} not fount.", configFile);
    G4Exception("WaterPhantom", "ParseTomlConfig", FatalErrorInArgument, "");
  }

  std::string configObjDetector("Detector");
  std::string configObjScoring("Scoring");
  if (!configPrefix.empty()) {  // here it's assummed that the config data is given with prefixed name
    configObjDetector.insert(0, configPrefix + "_");
    configObjScoring.insert(0, configPrefix + "_");
  } else {
    G4String msg = "The configuration PREFIX is not defined";
    FATAL_GEO(msg.data());
    G4Exception("WaterPhantom", "ParseTomlConfig", FatalErrorInArgument, msg);
  }

  auto config = toml::parse_file(configFile);

  ///
  m_sizeX = config[configObjDetector]["Size"][0].value_or(0.0);
  m_sizeY = config[configObjDetector]["Size"][1].value_or(0.0);
  m_sizeZ = config[configObjDetector]["Size"][2].value_or(0.0);
  m_centrePositionX = config[configObjDetector]["Centre"][0].value_or(0.0);
  m_centrePositionY = config[configObjDetector]["Centre"][1].value_or(0.0);
  m_centrePositionZ = config[configObjDetector]["Centre"][2].value_or(0.0);
  m_detectorVoxelizationX = config[configObjDetector]["Voxelization"][0].value_or(0);
  m_detectorVoxelizationY = config[configObjDetector]["Voxelization"][1].value_or(0);
  m_detectorVoxelizationZ = config[configObjDetector]["Voxelization"][2].value_or(0);
  ///
  m_phantomMedium = config[configObjDetector]["Medium"].value_or("G4_WATER");

  auto env_pos_x = Service<ConfigSvc>()->GetValue<double>("PatientGeometry", "PatientIsocentreX");
  auto env_pos_y = Service<ConfigSvc>()->GetValue<double>("PatientGeometry", "PatientIsocentreY");
  auto env_pos_z = Service<ConfigSvc>()->GetValue<double>("PatientGeometry", "PatientIsocentreZ");

  ///
  m_watertankScoring = config[configObjScoring]["FullVolume"].value_or(true);
  m_farmerScoring = config[configObjScoring]["FarmerDoseCalibration"].value_or(false);

}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Loads default parameterization for the water phantom.
 *
 * This implementation is a placeholder and always returns true without performing any action.
 *
 * @return G4bool Always returns true.
 */
G4bool WaterPhantom::LoadDefaultParameterization() { return true; }

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Logs information about the water phantom's size and position.
 *
 * Outputs the phantom's dimensions, the patient isocenter coordinates, and the phantom's center position within the simulation environment.
 */
void WaterPhantom::WriteInfo() {
  auto configSvc = Service<ConfigSvc>();
  G4String info;
  info = "Size of the water phantom: " + std::to_string(m_sizeX / cm) + " x " + std::to_string(m_sizeY / cm) + " x " + std::to_string(m_sizeZ / cm) + " [cm^3]";
  G4ThreeVector translation(configSvc->GetValue<double>("PatientGeometry", "PatientIsocentreX"), configSvc->GetValue<double>("PatientGeometry", "PatientIsocentreY"),
                            configSvc->GetValue<double>("PatientGeometry", "PatientIsocentreZ"));
  INFO_GEO(info);
  info = "Centre of the water phantom environment: (" + std::to_string(translation.getX() / cm) + "," + std::to_string(translation.getX() / cm) + "," +
         std::to_string(translation.getX() / cm) + ") [cm]\n";
  INFO_GEO(info);

  G4ThreeVector centre(m_centrePositionX * mm, m_centrePositionY * mm, m_centrePositionZ * mm);
  INFO_GEO("Centre of the water phantom (within the phantom world environment): {} [cm]\n", centre / cm);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Destroys the WaterPhantom physical volume and releases associated resources.
 *
 * Deletes the phantom's physical volume if it exists and resets the internal pointer to nullptr.
 */
void WaterPhantom::Destroy() {
  INFO_GEO("Destroying the WaterPhantom volume.");
  auto phantomVolume = GetPhysicalVolume();
  if (phantomVolume) {
    delete phantomVolume;
    SetPhysicalVolume(nullptr);
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Loads the water phantom parameterization from a TOML configuration file if available, or applies default parameters.
 *
 * @return G4bool Always returns true.
 */
G4bool WaterPhantom::LoadParameterization() {
  // Configurable::ValidateConfig();
  if (IsTomlConfigExists()) {
    ParseTomlConfig();
  } else {
    LoadDefaultParameterization();
  }
  return true;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs the water phantom geometry and places it in the simulation world.
 *
 * Creates a box-shaped water phantom with configured dimensions and material, places it at the specified center position within the parent world volume, assigns a dedicated Geant4 region with production cuts, and sets the phantom's volume for scoring.
 *
 * @param parentWorld The parent world physical volume in which the phantom is placed.
 */
void WaterPhantom::Construct(G4VPhysicalVolume* parentWorld) {
  LoadParameterization();

  auto medium = Service<ConfigSvc>()->GetValue<G4MaterialSPtr>("MaterialsSvc", m_phantomMedium);

  // create a phantom box filled with water, with given side dimensions
  auto waterPhantomBox = new G4Box("waterPhantomBox", m_sizeX / 2., m_sizeY / 2., m_sizeZ / 2.);

  auto waterPhantomLV = new G4LogicalVolume(waterPhantomBox, medium.get(), "waterPhantomLV");

  // the placement of phantom center in the gantry (global) coordinate system that is managed by PatientGeometry class
  // here we locate the phantom box in the center of envelope box created in PatientGeometry:
  SetPhysicalVolume(
      new G4PVPlacement(nullptr, G4ThreeVector(m_centrePositionX * mm, m_centrePositionY * mm, m_centrePositionZ * mm), "WaterPhantomPV", waterPhantomLV, parentWorld, false, 0));

  // Region for cuts
  auto regVol = new G4Region("waterPhantomR");
  auto cuts = new G4ProductionCuts;
  cuts->SetProductionCut(2.5 * mm);
  regVol->SetProductionCuts(cuts);
  waterPhantomLV->SetRegion(regVol);
  regVol->AddRootLogicalVolume(waterPhantomLV);

  // Set volume for scoring purposes
  SetVolume(m_sizeX * m_sizeY * m_sizeZ);  // Volume in mm^3
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Updates the water phantom state.
 *
 * Currently a placeholder that always returns true.
 *
 * @return G4bool Always returns true.
 */
G4bool WaterPhantom::Update() {
  // TODO implement me.
  return true;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Initializes the sensitive detector for the water phantom if it does not already exist.
 *
 * Creates a new `WaterPhantomSD` sensitive detector positioned at the combined translation of the phantom and its parent physical volumes, and assigns it to the phantom.
 */
void WaterPhantom::ConstructSensitiveDetector() {
  if (m_patientSD.Get() == 0) {
    // TODO: To be veryfied
    auto centre = GetPhysicalVolume()->GetTranslation();
    centre += GetParentPtr()->GetPhysicalVolume()->GetTranslation();
    m_patientSD.Put(new WaterPhantomSD("PhantomSD", centre));

  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Creates a full-volume scoring region for the water phantom.
 *
 * Initializes the sensitive detector if necessary and adds a scoring volume covering the entire phantom, using the specified voxelization parameters.
 *
 * @param name Name assigned to the scoring volume.
 */
void WaterPhantom::ConstructFullVolumeScoring(const G4String& name) {
  if (m_patientSD.Get() == 0) ConstructSensitiveDetector();
  auto patientSD = m_patientSD.Get();
  auto envBox = dynamic_cast<G4Box*>(GetPhysicalVolume()->GetLogicalVolume()->GetSolid());
  patientSD->AddScoringVolume(name, name, *envBox, m_detectorVoxelizationX, m_detectorVoxelizationY, m_detectorVoxelizationZ);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Adds a Farmer chamber-shaped scoring volume to the sensitive detector.
 *
 * Creates a scoring volume with the dimensions of a Farmer ionization chamber (5.3 mm × 5.3 mm × 40 cm) and assigns it to the sensitive detector with the specified name, using a voxelization of 1 × 1 × 200.
 *
 * @param name The identifier for the scoring volume.
 */
void WaterPhantom::ConstructFarmerVolumeScoring(const G4String& name) {
  if (m_patientSD.Get() == 0) ConstructSensitiveDetector();
  auto patientSD = m_patientSD.Get();
  auto farmerBox = G4Box(name + "Box", 2.65 * mm, 2.65 * mm, 20 * cm);
  patientSD->AddScoringVolume(name, name, farmerBox, 1, 1, 200);
}
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Sets up and assigns the sensitive detector for the water phantom logical volume.
 *
 * If not already created, constructs the sensitive detector and adds scoring volumes for full phantom and/or Farmer chamber scoring as configured. Assigns the sensitive detector to the "waterPhantomLV" logical volume.
 */
void WaterPhantom::DefineSensitiveDetector() {
  if (m_patientSD.Get() == 0) {
    if (m_watertankScoring) {
      ConstructFullVolumeScoring("WaterTank");
    }
    if (m_farmerScoring) {
      ConstructFarmerVolumeScoring("Farmer30013");
    }
    //
    VPatient::SetSensitiveDetector("waterPhantomLV", m_patientSD.Get());
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Generates a map of hashed voxel or cell indices to VoxelHit objects for scoring data.
 *
 * Depending on the specified scoring type, returns a map where each key is a hashed index representing either a voxel (for voxel-based scoring) or the entire phantom cell (for cell-based scoring), and each value is a VoxelHit containing position, ID, volume, and mass information. If voxelization is not available for the requested scoring volume, returns an empty map.
 *
 * @param scoring_name Name of the scoring volume.
 * @param type Scoring type (voxel-based or cell-based).
 * @return std::map<std::size_t, VoxelHit> Map from hashed indices to initialized VoxelHit objects.
 */
std::map<std::size_t, VoxelHit> WaterPhantom::GetScoringHashedMap(const G4String& scoring_name, Scoring::Type type) const {
  std::map<std::size_t, VoxelHit> hashed_map_scoring;

  auto Medium = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", m_phantomMedium);

  std::string hashedPhantomString = "000";

  if (type == Scoring::Type::Voxel) {
    auto sv = GetSD()->GetRunCollectionReferenceScoringVolume(scoring_name, true);
    if (sv == nullptr) return hashed_map_scoring;  // no voxelisation for this volume, return empty map
    auto centre = G4ThreeVector(m_centrePositionX * mm, m_centrePositionY * mm, m_centrePositionZ * mm);

    for (int ix = 0; ix < sv->m_nVoxelsX; ix++) {
      for (int iy = 0; iy < sv->m_nVoxelsY; iy++) {
        for (int iz = 0; iz < sv->m_nVoxelsZ; iz++) {
          auto voxelHash = svc::getHashedStrFromIndexes({0, 0, 0, ix, iy, iz});
          hashed_map_scoring[voxelHash] = VoxelHit();
          auto voxelCentre = sv->GetVoxelCentre(ix, iy, iz);
          // std::cout << voxelCentre.getZ() << std::endl;
          hashed_map_scoring[voxelHash].SetCentre(voxelCentre);
          hashed_map_scoring[voxelHash].SetGlobalCentre(centre);
          hashed_map_scoring[voxelHash].SetId(ix, iy, iz);
          hashed_map_scoring[voxelHash].SetGlobalId(0, 0, 0);
          hashed_map_scoring[voxelHash].SetStoreTracks(Service<ConfigSvc>()->GetValue<bool>("RunSvc", "StoreTracks"));
          hashed_map_scoring[voxelHash].SetVolume(sv->GetVoxelVolume());
          hashed_map_scoring[voxelHash].SetMass(Medium->GetDensity() * sv->GetVoxelVolume());
        }  // z
      }  // y
    }  // x
  } else if (type == Scoring::Type::Cell) {
    // auto phantomHash = std::hash<std::string>{}(hashedPhantomString);
    auto phantomHash = svc::getHashedStrFromIndexes({0, 0, 0});
    hashed_map_scoring[phantomHash] = VoxelHit();
    auto centre = G4ThreeVector(m_centrePositionX * mm, m_centrePositionY * mm, m_centrePositionZ * mm);
    hashed_map_scoring[phantomHash].SetCentre(centre);
    hashed_map_scoring[phantomHash].SetGlobalCentre(centre);
    hashed_map_scoring[phantomHash].SetId(0, 0, 0);
    hashed_map_scoring[phantomHash].SetGlobalId(0, 0, 0);  // Id == GlobalId
    hashed_map_scoring[phantomHash].SetStoreTracks(Service<ConfigSvc>()->GetValue<bool>("RunSvc", "StoreTracks"));
    auto volume = m_sizeX * m_sizeY * m_sizeZ;
    hashed_map_scoring[phantomHash].SetVolume(volume);
    hashed_map_scoring[phantomHash].SetMass(Medium->GetDensity() * volume);
  }

  return hashed_map_scoring;
}
