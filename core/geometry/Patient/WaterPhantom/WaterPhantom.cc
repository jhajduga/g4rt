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
 * @brief Load water-phantom parameters from a TOML configuration file.
 *
 * Parses the configured TOML file (using the configured prefix) and populates the WaterPhantom's
 * geometry, material, and scoring member variables.
 *
 * The following keys are read from the TOML sections "<prefix>_Detector" and "<prefix>_Scoring":
 * - Detector.Size: three-element array -> m_sizeX, m_sizeY, m_sizeZ (defaults to 0.0 for missing elements)
 * - Detector.Centre: three-element array -> m_centrePositionX, m_centrePositionY, m_centrePositionZ (defaults to 0.0)
 * - Detector.Voxelization: three-element array -> m_detectorVoxelizationX, m_detectorVoxelizationY, m_detectorVoxelizationZ (defaults to 0)
 * - Detector.Medium: string -> m_phantomMedium (defaults to "G4_WATER")
 * - Scoring.FullVolume: boolean -> m_watertankScoring (defaults to true)
 * - Scoring.FarmerDoseCalibration: boolean -> m_farmerScoring (defaults to false)
 *
 * Preconditions:
 * - A TOML configuration file must be available (as returned by GetTomlConfigFile()).
 * - A non-empty TOML configuration prefix must be provided (as returned by GetTomlConfigPrefix()).
 *
 * Error behavior:
 * - If the configuration file is missing or the prefix is empty, a Geant4 exception (FatalErrorInArgument)
 *   is raised.
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
 * @brief Log the water phantom's geometric summary.
 *
 * Logs the phantom outer dimensions (in cm), the environment patient isocentre coordinates (in cm),
 * and the phantom's centre position within the phantom/world environment (in cm).
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
 * @brief Load parameterization for the water phantom from configuration.
 *
 * Loads parameters from a TOML configuration file when one is present; otherwise
 * applies the built-in default parameterization. This function applies the
 * parsed or default settings to the instance (side effects on member state).
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
 * @brief Ensure a WaterPhantom sensitive detector exists and create it if missing.
 *
 * If no sensitive detector is currently attached, constructs a WaterPhantomSD named
 * "PhantomSD" positioned at the sum of the phantom's and its parent volume's translations
 * and stores it in m_patientSD.
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
 * @brief Add a full-volume, voxelized scoring volume that covers the entire water phantom.
 *
 * Ensures the water phantom sensitive detector exists, retrieves the phantom outer solid (must be a G4Box),
 * and registers that box as a scoring volume using the phantom's configured voxelization (m_detectorVoxelizationX/Y/Z).
 *
 * @param name Identifier used for the scoring volume within the sensitive detector.
 */
void WaterPhantom::ConstructFullVolumeScoring(const G4String& name) {
  if (m_patientSD.Get() == 0) ConstructSensitiveDetector();
  auto patientSD = m_patientSD.Get();
  auto envBox = dynamic_cast<G4Box*>(GetPhysicalVolume()->GetLogicalVolume()->GetSolid());
  patientSD->AddScoringVolume(name, name, *envBox, m_detectorVoxelizationX, m_detectorVoxelizationY, m_detectorVoxelizationZ);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Add a Farmer-type scoring volume to the patient sensitive detector.
 *
 * Ensures a WaterPhantom sensitive detector exists, then registers a Farmer-shaped
 * scoring volume (5.3 mm × 5.3 mm × 40 cm) with the given name. The scoring
 * volume is defined as a box solid and is added using a voxelization of
 * 1 × 1 × 200 (depth segmented into 200 voxels).
 *
 * @param name Identifier for the scoring volume.
 */
void WaterPhantom::ConstructFarmerVolumeScoring(const G4String& name) {
  if (m_patientSD.Get() == 0) ConstructSensitiveDetector();
  auto patientSD = m_patientSD.Get();
  auto farmerBox = G4Box(name + "Box", 2.65 * mm, 2.65 * mm, 20 * cm);
  patientSD->AddScoringVolume(name, name, farmerBox, 1, 1, 200);
}
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Ensure the water phantom has an attached sensitive detector and configured scoring volumes.
 *
 * If a sensitive detector is not already present, this creates the configured scoring volumes
 * and attaches the resulting sensitive detector to the logical volume named "waterPhantomLV".
 *
 * @details
 * - Adds a full-volume scoring named "WaterTank" when m_watertankScoring is true.
 * - Adds a Farmer-type scoring named "Farmer30013" when m_farmerScoring is true.
 * - No action is taken if a sensitive detector is already assigned.
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
 * @brief Build a map from hashed indices to initialized VoxelHit entries for a scoring volume.
 *
 * For Scoring::Type::Voxel, iterates all voxels of the scoring volume named by scoring_name and
 * creates a VoxelHit per voxel with centre, global centre (phantom centre), local IDs, store-tracks flag,
 * voxel volume and mass (material density × voxel volume). If the requested scoring volume has no
 * voxelization, an empty map is returned.
 *
 * For Scoring::Type::Cell, produces a single VoxelHit representing the entire phantom cell with its
 * centre, IDs set to (0,0,0), volume equal to the phantom volume, and mass computed from the
 * phantom material density.
 *
 * @param scoring_name Name of the scoring volume to query (used for voxel-based scoring lookup).
 * @param type Scoring type: Voxel for per-voxel entries or Cell for a single phantom-level entry.
 * @return std::map<std::size_t, VoxelHit> Map from hashed indices to pre-populated VoxelHit objects.
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
