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
 * @brief Destructor for the WorldConstruction class.
 *
 * Unregisters the configuration service for this instance and deletes the beam monitoring object.
 */
WorldConstruction::~WorldConstruction() {
  configSvc()->Unregister(thisConfig()->GetName());
  delete m_beamMonitoring;
  // this->Destroy();
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the singleton instance of the WorldConstruction class.
 *
 * Ensures only one instance of WorldConstruction exists, managed for the lifetime of the application.
 *
 * @return Pointer to the singleton WorldConstruction instance.
 */
WorldConstruction *WorldConstruction::GetInstance() {
  static auto instance = new WorldConstruction(); // It's being released by G4GeometryManager
  return instance;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Defines configuration parameters for the simulation world.
 *
 * Registers all configurable units and parameters required for constructing and managing the simulation world, including geometry, isocentre, phase space, rotation, and parameterized volumes. Sets up their default values using the base configuration mechanism.
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
 * @brief Sets default configuration values for the specified world construction parameter.
 *
 * Assigns a default value to the given configuration unit, such as world size, isocentre position, phase space parameters, rotation, or parameterized volumes. Used to initialize or reset configuration settings for the simulation world environment.
 *
 * @param unit The name of the configuration parameter to set to its default value.
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
 * @brief Destroys the world geometry and its submodules, preparing for cleanup.
 *
 * Releases geometry resources by opening the geometry for modification, destroys gantry and phantom environments if present, and unregisters the configuration for this instance.
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
 * @brief Returns the current physical volume representing the simulation world.
 *
 * @return Pointer to the world physical volume.
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
 * @brief Constructs and initializes world submodules based on configuration flags.
 *
 * Creates and attaches the gantry geometry, patient geometry, phase space saving planes, and beam monitoring modules to the world volume if their corresponding configuration options are enabled.
 *
 * @param parentPV Pointer to the parent physical volume to which submodules are attached.
 * @return true after all enabled submodules are constructed.
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
 * @brief Updates the world geometry state.
 *
 * This implementation performs no action and always returns true.
 *
 * @return G4bool Always returns true.
 */
G4bool WorldConstruction::Update() { return true; } // override

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Updates the world geometry for a specific run.
 *
 * Updates the phantom environment geometry if it exists. Returns `false` if the update fails; otherwise, returns `true`.
 *
 * @param runId The identifier for the current run.
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
 * @brief Checks for overlaps in the world geometry up to three levels of daughter volumes.
 *
 * Iterates through the world volume's daughters, their daughters, and their daughters' daughters, invoking `CheckOverlaps()` on each to detect and report any geometry overlaps.
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
 * @brief Indicates whether a new geometry has been created.
 *
 * Currently unimplemented; always returns false.
 *
 * @return false Always returns false as new geometry creation is not implemented.
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
 * @brief Exports the specified world geometry to a GDML file.
 *
 * Writes the geometry of the physical volume identified by `worldName` to a GDML file at the given path and file name. If the specified world volume is not found, logs an error and returns the intended file path without exporting.
 *
 * @param path Directory where the GDML file will be saved.
 * @param fileName Name of the GDML file to create.
 * @param worldName Name of the world volume to export.
 * @return std::string Full path to the exported GDML file.
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
 * @brief Writes informational output for the gantry and phantom environments if they exist.
 *
 * Calls the `WriteInfo()` method on the gantry and phantom geometry modules to output their configuration or status details.
 */
void WorldConstruction::WriteInfo(){
  if(m_gantryEnv) m_gantryEnv->WriteInfo();
  if(m_phantomEnv) m_phantomEnv->WriteInfo();
}
