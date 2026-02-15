#include "WorldConstruction.hh"
#include "Services.hh"
#include "PatientGeometry.hh"
#include "SavePhSpConstruction.hh"
#include "LinacGeometry.hh"
#include "G4GDMLParser.hh"
#include "G4GeometryManager.hh"
#include "G4Box.hh"
#include "G4Run.hh"
#include "BeamMonitoring.hh"
#include "MyGeometryTolerance.hh"

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs a WorldConstruction object for managing the simulation world.
 *
 * Initializes the WorldConstruction instance by invoking base class constructors for IPhysicalVolume and Configurable. Does not perform configuration or geometry creation during construction.
 */
WorldConstruction::WorldConstruction()
  :IPhysicalVolume("WorldConstruction"), Configurable("WorldConstruction"){
  // Configure();
  // // build a geometry
  // Create();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Clean up a WorldConstruction instance.
 *
 * Unregisters this instance from the configuration service and releases the beam
 * monitoring object owned by the instance.
 *
 * @note The top-level world physical volume is not deleted here; its lifetime is
 * managed by the Geant4 geometry manager (G4RunManager).
 */
WorldConstruction::~WorldConstruction() {
  configSvc()->Unregister(thisConfig()->GetName());
  delete m_beamMonitoring;
  // this->Destroy();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Return the singleton WorldConstruction instance.
 *
 * Creates and returns the single global WorldConstruction used by the application.
 * The instance is allocated once on first call and retained for the program lifetime;
 * ownership is effectively managed by Geant4's geometry manager.
 *
 * @return WorldConstruction* Non-null pointer to the singleton instance.
 */
WorldConstruction *WorldConstruction::GetInstance() {
  static auto instance = new WorldConstruction(); // It's being released by G4GeometryManager
  return instance;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Register and initialize all configurable units used by the simulation world.
 *
 * Defines the named configuration units required to construct and manage the world geometry
 * (world size, isocentre, coordinate transforms, phase‑space settings, rotations and
 * parameterized volumes) and then populates them with their default values by invoking
 * the base Configurable::DefaultConfig().
 *
 * This method has the side effect of registering these units with the configuration
 * subsystem so they become available for runtime queries and overrides.
 */
void WorldConstruction::Configure() {
  G4cout << "\n\n[INFO]::  Default configuration of the " << thisConfig()->GetName() << G4endl;

  DefineUnit<G4ThreeVector>("WorldSize");
  DefineUnit<G4ThreeVector>("Isocentre");
  DefineUnit<G4ThreeVector>("IsoToSimTransformation");
  DefineUnit<G4double>("SourceZPosition");
  DefineUnit<G4bool>("ForcePhaseSpaceBeforeJaws");
  DefineUnit<G4ThreeVector>("centrePhaseSpace");
  DefineUnit<G4ThreeVector>("halfSizePhaseSpace");
  DefineUnit<G4bool>("SavePhaseSpace");
  DefineUnit<G4bool>("StopAtPhaseSpace");
  DefineUnit<G4String>("PhaseSpaceOutFile");
  DefineUnit<G4int>("max_N_particles_in_PhSp_File");
  DefineUnit<G4int>("nMaxParticlesInRamPlanePhaseSpace");
  DefineUnit<G4bool>("Rotate90Y");
  DefineUnit<G4double>("rotationX");
  DefineUnit<std::set<std::string> *>("ParameterizedVolumes");

  Configurable::DefaultConfig();   // setup the default configuration for all defined units/parameters
  // Configurable::PrintConfig();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Set a default value for a named world-construction configuration unit.
 *
 * Initializes a single configuration entry identified by its name to a sensible default
 * used by the world construction subsystem. The function updates the active configuration
 * via thisConfig()->SetValue(...) for the requested unit name.
 *
 * Supported unit names and their default meanings:
 * - "Label": human-readable label ("World Construction Environment").
 * - "WorldSize": world box full dimensions (G4ThreeVector) in mm — default (2000,2000,2000).
 * - "Isocentre": isocentre position (G4ThreeVector) in mm — default (0,0,0).
 * - "IsoToSimTransformation": transform vector (G4ThreeVector) — default (0,0,0).
 * - "SourceZPosition": source Z position (G4double) in mm — default 1000.
 * - "centrePhaseSpace": center of phase-space plane (G4ThreeVector) in mm — default (0,0,550).
 * - "halfSizePhaseSpace": half-size of phase-space plane (G4ThreeVector) in mm — default (200,200,1).
 * - "ForcePhaseSpaceBeforeJaws": force phase-space before jaws (G4bool) — default true.
 * - "PhaseSpaceOutFile": phase-space output filename (G4String) — default "PhSp_Acc1".
 * - "SavePhaseSpace": enable phase-space saving (G4bool) — default false.
 * - "StopAtPhaseSpace": stop particle at phase-space (G4bool) — default false.
 * - "max_N_particles_in_PhSp_File": max particles in phase-space file (G4int) — default 10,000,000.
 * - "nMaxParticlesInRamPlanePhaseSpace": max particles buffered in RAM (G4int) — default 100,000.
 * - "Rotate90Y": rotate accelerator 90° about Y (G4bool) — default false.
 * - "rotationX": rotation angle about X (G4double) in degrees — default 0.
 * - "ParameterizedVolumes": set of parameterized volume names (std::set<std::string>*) — default empty set.
 *
 * @param unit The configuration unit name to initialize. If the name is unrecognized the function
 *             performs no action.
 */
void WorldConstruction::DefaultConfig(const std::string &unit) {

  // Module name
  if (unit.compare("Label") == 0) thisConfig()->SetValue(unit, std::string("World Construction Environment"));

  // The full length of the World in [mm]
  // - the actual WorldBox is defined as cuboid with size WorldSize.X/2  WorldSize.Y/2 x WorldSize.Z/2)
  if (unit.compare("WorldSize") == 0) 
    thisConfig()->SetValue(unit, G4ThreeVector(2000., 2000., 2000.));

  if (unit.compare("Isocentre") == 0) 
    thisConfig()->SetValue(unit, G4ThreeVector(0.,0.,0.));

  if (unit.compare("IsoToSimTransformation") == 0){ 
    // auto worldSize = thisConfig()->GetValue<G4ThreeVector>("WorldSize");
    // auto isoCentre = thisConfig()->GetValue<G4ThreeVector>("Isocentre");
    // thisConfig()->SetValue(unit, (worldSize-isoCentre)*G4ThreeVector(0.5,0.5,-0.5));
    thisConfig()->SetValue(unit, G4ThreeVector(0.,0.,0.));
  }

  // describe me.
  if (unit.compare("SourceZPosition") == 0) 
    thisConfig()->SetValue(unit, G4double(1000.));  // [mm]

  // position of the centre of the plane phase space:
  if (unit.compare("centrePhaseSpace") == 0)
    thisConfig()->SetValue(unit, G4ThreeVector(0. * mm, 0. * mm, 550. * mm));  // [mm]

  // half size of the plane phase space
  if (unit.compare("halfSizePhaseSpace") == 0)
    thisConfig()->SetValue(unit, G4ThreeVector(200. * mm, 200. * mm, 1. * mm));  // [mm]

  // describe me.
  if (unit.compare("ForcePhaseSpaceBeforeJaws") == 0) thisConfig()->SetValue(unit, G4bool(true));

  // full file name of the phase space
  if (unit.compare("PhaseSpaceOutFile") == 0) thisConfig()->SetValue(unit, G4String("PhSp_Acc1"));

  // true if to save the phase space
  if (unit.compare("SavePhaseSpace") == 0) thisConfig()->SetValue(unit, G4bool(false));

  // true if to kill the particle at the phase space
  if (unit.compare("StopAtPhaseSpace") == 0) thisConfig()->SetValue(unit, G4bool(false));

  //  maximum particle number stored in the phase space file
  if (unit.compare("max_N_particles_in_PhSp_File") == 0) thisConfig()->SetValue(unit, G4int(10000000));

  // maximum particle number stored in RAM before saving - for phase space
  if (unit.compare("nMaxParticlesInRamPlanePhaseSpace") == 0) thisConfig()->SetValue(unit, G4int(100000));

  // rotate the accelerator of 90 deg around the Y axis
  if (unit.compare("Rotate90Y") == 0) thisConfig()->SetValue(unit, G4bool(false));

  // Angle of rotation along X  [deg]
  // NOTE: this still need be reimplemented into single number in core code
  if (unit.compare("rotationX") == 0) thisConfig()->SetValue(unit, G4double(0.));

  // List of volumes in the whole world structure being parameterized using any type of VParameterization
  if (unit.compare("ParameterizedVolumes") == 0) thisConfig()->SetValue(unit, new std::set<std::string>{});
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Destroy the world geometry and tear down submodules.
 *
 * Opens the Geant4 geometry for modification, invokes Destroy() on constructed
 * submodules (gantry and phantom) if present, and unregisters this instance's
 * configuration from the configuration service. The top-level world physical
 * volume is not deleted here because its lifetime is managed by the Geant4
 * run manager.
 */
void WorldConstruction::Destroy() {

            INFO_GEO("Destroing the World volumes... ");
  auto physicalWorldVolume = GetPhysicalVolume();
  if (physicalWorldVolume) {
    G4GeometryManager::GetInstance()->OpenGeometry();
    if (m_gantryEnv) m_gantryEnv->Destroy();
    if (m_phantomEnv) m_phantomEnv->Destroy();
    // NOTE: The actual m_parentPV is deleted by G4RunManager destructor and
    //       G4RunManager::DeleteUserInitializations()
    //       hence it has to be released before manual deletion
    // delete GetPhysicalVolume();
    // GetPhysicalVolume( nullptr );
  }
  configSvc()->Unregister(thisConfig()->GetName());
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Get the current physical volume representing the simulation world.
 *
 * @return G4VPhysicalVolume* Pointer to the world physical volume, or `nullptr` if no world has been created.
 */
G4VPhysicalVolume* WorldConstruction::Construct() { return GetPhysicalVolume(); }

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Creates and initializes the simulation world geometry.
 *
 * Builds the main world volume as a box filled with air, positions it at the configured isocentre, and constructs all configured submodules within the world. Updates the internal pointer to the world physical volume.
 *
 * @return true upon successful creation of the world geometry.
 */
bool WorldConstruction::Create() {
  // MyGeometryTolerance::ResetSurfaceTolerance(0.0005*mm);
  // create the world box
  auto worldSize = configSvc()->GetValue<G4ThreeVector>("WorldConstruction", "WorldSize");
  //auto Air = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "Usr_G4AIR20C");
  auto Air = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "Usr_G4AIR20C");
  auto worldB = new G4Box("worldG", worldSize.getX(), worldSize.getY(), worldSize.getZ());
  auto worldLV = new G4LogicalVolume(worldB, Air.get(), "worldLV", 0, 0, 0);
  auto isocentre = thisConfig()->GetValue<G4ThreeVector>("Isocentre");
  SetPhysicalVolume(new G4PVPlacement(0, isocentre, "worldPV", worldLV, 0, false, 0));
  m_worldPV = GetPhysicalVolume();

  ConstructWorldModules(m_worldPV);

  // auto treeDepth = GetWorldVolumesTreeDepth();
  // G4cout << "[DEBUG]::  WorldConstruction the tree depth " << treeDepth << G4endl;

  // auto nVolumes = GetNoOfWorldVolumes(-1);
  // G4cout << "[DEBUG]::  WorldConstruction no of component volumes -1 " << nVolumes << G4endl;

  // nVolumes = GetNoOfWorldVolumes(0);
  // G4cout << "[DEBUG]::  WorldConstruction no of component volumes 0 " << nVolumes << G4endl;

  // nVolumes = GetNoOfWorldVolumes(1);
  // G4cout << "[DEBUG]::  WorldConstruction no of component volumes 1 " << nVolumes << G4endl;

  // nVolumes = GetNoOfWorldVolumes(2);
  // G4cout << "[DEBUG]::  WorldConstruction no of component volumes 2 " << nVolumes << G4endl;

  // GetPhysicalVolume("DUMMY");

  return true;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Instantiate and attach configured world submodules.
 *
 * Constructs enabled submodules (gantry, patient/phantom, phase-space planes, and beam-monitoring)
 * and attaches them to the provided parent physical volume. Each created module is stored in the
 * class' member pointers (e.g., m_gantryEnv, m_phantomEnv, m_savePhSpEnv, m_beamMonitoring).
 *
 * The presence of each submodule is determined by configuration flags:
 * - GeoSvc::BuildLinac -> gantry (LinacGeometry)
 * - GeoSvc::BuildPatient -> patient/phantom (PatientGeometry)
 * - RunSvc::SavePhSp -> phase-space planes (SavePhSpConstruction)
 * - RunSvc::BeamAnalysis -> beam monitoring (BeamMonitoring)
 *
 * @param parentPV Parent physical volume to which submodules are attached.
 * @return true Always returns true after attempting construction of enabled submodules.
 */
bool WorldConstruction::ConstructWorldModules(G4VPhysicalVolume *parentPV) {
  // ___________________________________________________________________
  // create the gantry-world box
  if (configSvc()->GetValue<G4bool>("GeoSvc", "BuildLinac")){
    m_gantryEnv = LinacGeometry::GetInstance();
    if (m_gantryEnv) {
      m_gantryEnv->IPhysicalVolume::Construct(this);
    }
  } else {
            DEBUG_GEO("The gantry geometry is switched off...");
  }

  // ___________________________________________________________________
  // create the phantom-world box
  if (configSvc()->GetValue<G4bool>("GeoSvc", "BuildPatient")){
    m_phantomEnv = PatientGeometry::GetInstance();
    if (m_phantomEnv) {
      m_phantomEnv->IPhysicalVolume::Construct(this);
    }
  } else {
  
            DEBUG_GEO("The patient geometry is switched off... ");
  }

  // ___________________________________________________________________
  // create user-defined PhSp planes
  if (configSvc()->GetValue<bool>("RunSvc", "SavePhSp")) {
    m_savePhSpEnv = SavePhSpConstruction::GetInstance();
    if (m_savePhSpEnv) {
      m_savePhSpEnv->IPhysicalVolume::Construct(this);
    }
  }

  // ___________________________________________________________________
  // create beam monitoring planes
  if (configSvc()->GetValue<bool>("RunSvc", "BeamAnalysis")) {
    m_beamMonitoring = new BeamMonitoring();
    m_beamMonitoring->IPhysicalVolume::Construct(this);
  }
  return true;
}
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Defines sensitive detectors for all constructed submodules in the world.
 *
 * Calls the `DefineSensitiveDetector()` method on each submodule (gantry, phase space, phantom, and beam monitoring) if it exists.
 */
void WorldConstruction::ConstructSDandField() {

  if(m_gantryEnv) m_gantryEnv->DefineSensitiveDetector();
  if(m_savePhSpEnv) m_savePhSpEnv->DefineSensitiveDetector();
  if(m_phantomEnv) m_phantomEnv->DefineSensitiveDetector();
  if(m_beamMonitoring) m_beamMonitoring->DefineSensitiveDetector();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief No-op update that leaves the world geometry unchanged.
 *
 * Present for interface compatibility; performs no modifications.
 *
 * @return G4bool `true` indicating the update succeeded.
 */
G4bool WorldConstruction::Update() { return true; } // override

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Update world submodules for a specific run.
 *
 * Attempts to update the phantom environment (if present) for the provided run identifier.
 *
 * @param runId Identifier of the current run.
 * @return G4bool `true` if the update succeeds, `false` otherwise.
 */
G4bool WorldConstruction::Update(int runId) {

            INFO_GEO("Updating world geometry for run {}",runId);
  // if (thisConfig()->GetStatus()) {
  //   G4cout << "[INFO]:: World configuration has been updated..." << G4endl;
  //   for (auto param : thisConfig()->GetUnitsNames()) {
  //     if (thisConfig()->GetStatus(param)) {
  //       G4cout << "[INFO]:: Updated:: " << param << " : value will be printed here (implement me)." << G4endl;
  //     }
  //   }
  // }

  // if(thisConfig()->GetStatus("rotationX")){
  //   auto rotationX = thisConfig()->GetValue<G4double>("rotationX");
  //   //m_gantryEnv->rotateHead(rotationX);
  // }

//  if (m_gantryEnv) {
//    if (!m_gantryEnv->Update()) return false;
//  }

  if (m_phantomEnv) {
    if (!m_phantomEnv->Update()) return false;
  }


  return true;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Check for geometry overlaps in the world volume up to three daughter levels.
 *
 * Traverses the world physical volume's immediate daughters, their daughters, and their
 * daughters' daughters, calling each volume's CheckOverlaps() to report any overlapping
 * placements. Does not recurse beyond the third level of descendants.
 *
 * Preconditions:
 * - A valid world physical volume must be present (GetPhysicalVolume() must not be null).
 *
 * This function has no return value; overlap diagnostics are produced by the underlying
 * Geant4 CheckOverlaps() calls.
 */
void WorldConstruction::checkVolumeOverlap() {
  // loop inside all the daughters volumes
  //        bool bCheckOverlap;
  //        bCheckOverlap=false;

  int nSubWorlds, nSubWorlds2;
  for (int i = 0; i < (int) GetPhysicalVolume()->GetLogicalVolume()->GetNoDaughters(); i++) {
    GetPhysicalVolume()->GetLogicalVolume()->GetDaughter(i)->CheckOverlaps();
    nSubWorlds = (int) GetPhysicalVolume()->GetLogicalVolume()->GetDaughter(i)->GetLogicalVolume()->GetNoDaughters();
    for (int j = 0; j < nSubWorlds; j++) {
      GetPhysicalVolume()->GetLogicalVolume()->GetDaughter(i)->GetLogicalVolume()->GetDaughter(j)->CheckOverlaps();
      nSubWorlds2 = (int) GetPhysicalVolume()->GetLogicalVolume()
          ->GetDaughter(i)
          ->GetLogicalVolume()
          ->GetDaughter(j)
          ->GetLogicalVolume()
          ->GetNoDaughters();
      for (int k = 0; k < nSubWorlds2; k++) {
        GetPhysicalVolume()->GetLogicalVolume()
            ->GetDaughter(i)
            ->GetLogicalVolume()
            ->GetDaughter(j)
            ->GetLogicalVolume()
            ->GetDaughter(k)
            ->CheckOverlaps();
      }
    }
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Reports whether a new geometry was created since the last update.
 *
 * Currently unimplemented; this function always reports no new geometry.
 *
 * @return `true` if new geometry was created, `false` otherwise (currently always `false`).
 */
bool WorldConstruction::newGeometry() {
  G4cout << "[ERROR] :: WorldConstruction::newGeometry::Implement me." << G4endl;
  G4bool bNewGeometry = false;
  // G4bool bNewRotation = false;
  // G4bool bNewCentre = false;
  // bNewCentre = m_phantomEnv->applyNewCentre();
  // G4RotationMatrix *rmInv = gantryEnv->rotateGantry();
  // G4RotationMatrix *rmInv = acceleratorEnv->rotateHead();
  // if (rmInv != 0) {
  //   // CML2PrimaryGenerationAction::GetInstance()->setRotation(rmInv);
  //   bNewRotation = true;
  // }
  // if (bNewRotation || bNewCentre) {
  //   bNewGeometry = true;
  // }
  return bNewGeometry;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Export a world geometry to a GDML file.
 *
 * Exports the physical volume identified by |worldName| to a GDML file located at
 * path/filename. If a file already exists at that path it is removed before writing.
 * If |worldName| is non-empty but no matching physical volume is found, no export
 * is performed (an error is logged) and the intended file path is still returned.
 *
 * The function returns the full path that was used for the export (or intended export).
 *
 * @return std::string Full path to the GDML file (path + "/" + fileName).
 */
std::string WorldConstruction::ExportToGDML(const std::string& path, const std::string& fileName, const std::string& worldName) {
  auto file = path+"/"+fileName;
  svc::deleteFileIfExists(file);
  auto parserW = std::make_unique<G4GDMLParser>();
  // select the top world for the export begin from:
  auto worldToBeExported = GetPhysicalVolume(worldName);
  if (!worldName.empty() && !worldToBeExported) {
  ERROR_GEO("ExportToGDML:: Couldn't find the world of interest");
    return file;
  }

  INFO_GEO("Exporting: {}", worldToBeExported->GetName());
  
  // build and switch to the solid like volumes of parameterized worlds
  // TODO make this working for all geometry tree!
  // IPhysicalVolume::ParameterisationInstantiation(IParameterisation::EXPORT);

  parserW->Write(file, worldToBeExported);

  INFO_GEO("World exported to gdml file");

  // after the export, switch back the voxelization to the parameterized one.
  // TODO make this working for all geometry tree!
  // IPhysicalVolume::ParameterisationInstantiation(IParameterisation::SIMULATION);
  return file;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Emit informational output for the active gantry and phantom environments.
 *
 * If the gantry or phantom environment modules are present, their `WriteInfo()` methods
 * are invoked to produce configuration or status information.
 */
void WorldConstruction::WriteInfo(){
  if(m_gantryEnv) m_gantryEnv->WriteInfo();
  if(m_phantomEnv) m_phantomEnv->WriteInfo();
}