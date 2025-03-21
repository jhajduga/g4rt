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

////////////////////////////////////////////////////////////////////////////////
///
WorldConstruction::WorldConstruction()
  :IPhysicalVolume("WorldConstruction"), Configurable("WorldConstruction"),Logable("GeoAndScoring"){
  // Configure();
  // // build a geometry
  // Create();
}

////////////////////////////////////////////////////////////////////////////////
///
WorldConstruction::~WorldConstruction() {
  configSvc()->Unregister(thisConfig()->GetName());
  delete m_beamMonitoring;
  // this->Destroy();
}

////////////////////////////////////////////////////////////////////////////////
///
WorldConstruction *WorldConstruction::GetInstance() {
  static auto instance = new WorldConstruction(); // It's being released by G4GeometryManager
  return instance;
}

////////////////////////////////////////////////////////////////////////////////
///
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
///
void WorldConstruction::DefaultConfig(const std::string &unit) {

  // Module name
  if (unit.compare("Label") == 0) thisConfig()->SetValue(unit, std::string("World Construction Environment"));

  // The full length of the World in [mm]
  // - the actual WorldBox is defined as cuboid with size WorldSize.X/2  WorldSize.Y/2 x WorldSize.Z/2)
  if (unit.compare("WorldSize") == 0) 
    thisConfig()->SetValue(unit, G4ThreeVector(6000., 6000., 6000.));

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
///
void WorldConstruction::Destroy() {
  LOGSVC_INFO("Destroing the World volumes... ");
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
///
G4VPhysicalVolume* WorldConstruction::Construct() { return GetPhysicalVolume(); }

////////////////////////////////////////////////////////////////////////////////
///
bool WorldConstruction::Create() {

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
///
bool WorldConstruction::ConstructWorldModules(G4VPhysicalVolume *parentPV) {
  // ___________________________________________________________________
  // create the gantry-world box
  if (configSvc()->GetValue<G4bool>("GeoSvc", "BuildLinac")){
    m_gantryEnv = LinacGeometry::GetInstance();
    if (m_gantryEnv) {
      m_gantryEnv->IPhysicalVolume::Construct(this);
    }
  } else {
    LOGSVC_DEBUG('WorldConstruction:: The gantry geometry is switched off...')
  }

  // ___________________________________________________________________
  // create the phantom-world box
  if (configSvc()->GetValue<G4bool>("GeoSvc", "BuildPatient")){
    m_phantomEnv = PatientGeometry::GetInstance();
    if (m_phantomEnv) {
      m_phantomEnv->IPhysicalVolume::Construct(this);
    }
  } else {
    LOGSVC_DEBUG("[DEBUG]::WorldConstruction:: The patient geometry is switched off... ")
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
///
void WorldConstruction::ConstructSDandField() {

  if(m_gantryEnv) m_gantryEnv->DefineSensitiveDetector();
  if(m_savePhSpEnv) m_savePhSpEnv->DefineSensitiveDetector();
  if(m_phantomEnv) m_phantomEnv->DefineSensitiveDetector();
  if(m_beamMonitoring) m_beamMonitoring->DefineSensitiveDetector();
}

////////////////////////////////////////////////////////////////////////////////
///
G4bool WorldConstruction::Update() { return true; } // override

////////////////////////////////////////////////////////////////////////////////
///
G4bool WorldConstruction::Update(int runId) {
  LOGSVC_INFO("Updating world geometry for run {}",runId);
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
///
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
///
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
///
std::string WorldConstruction::ExportToGDML(const std::string& path, const std::string& fileName, const std::string& worldName) {
  auto file = path+"/"+fileName;
  svc::deleteFileIfExists(file);
  auto parserW = std::make_unique<G4GDMLParser>();
  // select the top world for the export begin from:
  auto worldToBeExported = GetPhysicalVolume(worldName);
  if (!worldName.empty() && !worldToBeExported) {
    LOGSVC_ERROR("ExportToGDML:: Couldn't find the world of interest");
    return file;
  }
  LOGSVC_INFO("Exporting: {}", worldToBeExported->GetName());
  
  // build and switch to the solid like volumes of parameterized worlds
  // TODO make this working for all geometry tree!
  // IPhysicalVolume::ParameterisationInstantiation(IParameterisation::EXPORT);

  parserW->Write(file, worldToBeExported);
  LOGSVC_INFO("World exported to gdml file");

  // after the export, switch back the voxelization to the parameterized one.
  // TODO make this working for all geometry tree!
  // IPhysicalVolume::ParameterisationInstantiation(IParameterisation::SIMULATION);
  return file;
}

////////////////////////////////////////////////////////////////////////////////
///
void WorldConstruction::WriteInfo(){
  if(m_gantryEnv) m_gantryEnv->WriteInfo();
  if(m_phantomEnv) m_phantomEnv->WriteInfo();
}
