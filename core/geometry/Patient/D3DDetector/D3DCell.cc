#include "D3DCell.hh"
#include "G4SystemOfUnits.hh"
#include "GeoSvc.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4SubtractionSolid.hh"
#include "G4ProductionCuts.hh"
#include "D3DCellSD.hh"
#include "ConfigSvc.hh"
#include "G4UserLimits.hh"
#include "NTupleEventAnalisys.hh"
#include "RunAnalysis.hh"
#include "colors.hh"
#include <vector>
#include "Services.hh"
#include "IbaImRT.hh"

namespace {
G4Mutex CellMutex = G4MUTEX_INITIALIZER;
}

G4ThreeVector D3DCell::SIZE = {10.0 * mm, 10.0 * mm, 10.0 * mm};

// G4double D3DCell::SIZE = 5.4 * mm;
// G4double D3DCell::SIZE = 2 * cm;

G4bool D3DCell::m_set_cell_scorer = true;
G4bool D3DCell::m_set_cell_voxelised_scorer = true;
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Enables or disables cell scoring for all D3DCell instances.
 *
 * Sets the static flag controlling whether cell-level scoring is active in the simulation.
 *
 * @param val If true, enables cell scoring; if false, disables it.
 */
void D3DCell::CellScorer(G4bool val) {
  m_set_cell_scorer = val;
  // if(!val){ // by default it's set to true
  //   Service<RunSvc>()->GetScoringTypes().erase(Scoring::Type::Cell);
  // }
}
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Enables or disables voxelized scoring for all D3DCell instances.
 *
 * When disabled, removes the voxel scoring type from the run service's scoring types.
 *
 * @param val Set to true to enable voxelized scoring, false to disable.
 */
void D3DCell::CellVoxelisedScorer(G4bool val) {
  m_set_cell_voxelised_scorer = val;
  if (!val) {  // by default it's set to true
    Service<RunSvc>()->GetScoringTypes().erase(Scoring::Type::Voxel);
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs a D3DCell with the specified label, global center position, and material name.
 *
 * Initializes the cell with a unique label, sets its global center coordinates, and assigns the material medium for geometry construction.
 *
 * @param label Unique identifier for the cell.
 * @param centre Global center position of the cell.
 * @param cellMediumName Name of the material medium for the cell.
 */
D3DCell::D3DCell(const G4String& label, const G4ThreeVector& centre, G4String cellMediumName) : VPatient(label), m_cell_medium(cellMediumName) { m_global_centre = centre; }

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Destructor for the D3DCell class.
 *
 * Cleans up resources by destroying the associated physical volume and deleting the step limit if allocated.
 */
D3DCell::~D3DCell() {
  Destroy();
  if (m_step_limit) delete m_step_limit;
}

/**
 * @brief Sets the cell's integer identifiers along the x, y, and z axes.
 *
 * @param x Identifier for the x-axis.
 * @param y Identifier for the y-axis.
 * @param z Identifier for the z-axis.
 */
void D3DCell::SetIDs(G4int x, G4int y, G4int z) {
  m_id_x = x;
  m_id_y = y;
  m_id_z = z;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Outputs a placeholder message indicating that cell information output is not yet implemented.
 */
void D3DCell::WriteInfo() { INFO_GEO("The Dose3D cell {} info: Implement me.", GetName()); }

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Deletes the physical volume associated with the D3DCell and resets its pointer.
 *
 * Frees resources by deleting the cell's physical volume if it exists and sets the internal pointer to null.
 */
void D3DCell::Destroy() {
  INFO_GEO("Destroing the D3DCell volume.");
  auto phantomVolume = GetPhysicalVolume();
  if (phantomVolume) {
    delete phantomVolume;
    SetPhysicalVolume(nullptr);
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Sets the number of voxels along a specified axis for voxelized scoring.
 *
 * @param axis The axis to set ('x', 'y', or 'z', case-insensitive).
 * @param nv The number of voxels to assign along the specified axis.
 */
void D3DCell::SetNVoxels(char axis, int nv) {
  switch (std::tolower(axis)) {
    case 'x':
      m_cell_voxelization_x = nv;
      break;
    case 'y':
      m_cell_voxelization_y = nv;
      break;
    case 'z':
      m_cell_voxelization_z = nv;
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs the 3D dose cell geometry and places it within the parent volume.
 *
 * Creates the cell's solid and logical volumes with the specified material, transforms and sets its position, assigns production cuts, and associates a simulation region. Updates the cell's global center and sets the cell volume.
 *
 * @param parentWorld The parent physical volume in which the cell is placed.
 */
void D3DCell::Construct(G4VPhysicalVolume* parentWorld) {
  // std::cout << "[INFO]:: D3DCell construction... " << std::endl;
  auto label = GetName();
  const auto& size = D3DCell::SIZE;
  // std::cout << "Parent world was set. " << std::endl;

  auto Medium = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", m_cell_medium);
  // create a cell box filled with PMMA, with given side dimensions
  auto dose3dCellBox = new G4Box(label + "Box", size.getX() / 2., size.getY() / 2., size.getZ() / 2.);
  auto dose3dCellLV = new G4LogicalVolume(dose3dCellBox, Medium.get(), label + "LV");
  // the placement of phantom center in the gantry (global) coordinate system that is managed by PatientGeometry class
  // here we locate the phantom box in the center of envelope box created in PatientGeometry:
  m_centre = svc::transformPosition(m_global_centre, this, svc::Transform::GlobalToLocal);
  SetPhysicalVolume(new G4PVPlacement(nullptr, m_centre, label + "PV", dose3dCellLV, parentWorld, false, 0));
  auto env_pos_x = Service<ConfigSvc>()->GetValue<double>("PatientGeometry", "PatientIsocentreX");
  auto env_pos_y = Service<ConfigSvc>()->GetValue<double>("PatientGeometry", "PatientIsocentreY");
  auto env_pos_z = Service<ConfigSvc>()->GetValue<double>("PatientGeometry", "PatientIsocentreZ");
  auto izocentre = G4ThreeVector(env_pos_x, env_pos_y, env_pos_z);
  m_global_centre = m_global_centre - IbaImRT::IbaToLocalTranslation + izocentre;
  G4cout << "[DEBUG]:: IBA: " << IbaImRT::IbaToLocalTranslation << " Izocentre: " << izocentre << G4endl;
  G4cout << "[DEBUG]:: D3DCell:: creating cell: " << label << " with position: " << m_centre << "(local), " << m_global_centre << "(global)" << G4endl;

  // std::cout << "[DEBUG]:: D3DCell:: creating cuts " << label <<"_G4RegionCuts" << G4endl;
  auto regVol = new G4Region(label + "_G4RegionCuts");
  auto cuts = new G4ProductionCuts;
  cuts->SetProductionCut(0.1 * mm);
  regVol->SetProductionCuts(cuts);
  dose3dCellLV->SetRegion(regVol);
  regVol->AddRootLogicalVolume(dose3dCellLV);

  // G4UserLimits* userLimits = new G4UserLimits();
  // userLimits->SetMaxAllowedStep(1.0 * um);
  // dose3dCellLV->SetUserLimits(userLimits);

  SetVolume(size.getX() * size.getY() * size.getZ());
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Placeholder update method for the D3DCell.
 *
 * Always returns true. Intended for future extension to update cell state if needed.
 *
 * @return G4bool Always true.
 */
G4bool D3DCell::Update() { return true; }

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Determines if voxelized scoring is enabled for a given run collection.
 *
 * Checks whether the sensitive detector has voxelized scoring configured for the specified run collection.
 *
 * @param run_collection The name of the run collection to query.
 * @return true if voxelized scoring is enabled for the run collection; false otherwise.
 */
bool D3DCell::IsRunCollectionScoringVolumeVoxelised(const G4String& run_collection) const {
  if (GetSD()->GetRunCollectionReferenceScoringVolume(run_collection, true)) return true;
  return false;
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Defines and attaches a sensitive detector to the cell volume.
 *
 * Creates a `D3DCellSD` sensitive detector for the cell if one does not already exist, configures it with the cell's label, global center, and IDs, and sets up scoring volumes with the appropriate voxelization parameters. Registers the sensitive detector with the logical volume and the Geant4 SD manager.
 */
void D3DCell::DefineSensitiveDetector() {
  G4AutoLock lock(&CellMutex);
  if (m_patientSD.Get() == 0) {
    auto pv = GetPhysicalVolume();
    auto centre = m_global_centre;  // wrap this to VPatient::GetGlobalTranslation

    DEBUG_GEO("Construct SD >> current centre {} {} {}", centre.x(), centre.y(), centre.z());

    auto envBox = dynamic_cast<G4Box*>(pv->GetLogicalVolume()->GetSolid());
    auto label = GetName();
    m_patientSD.Put(new D3DCellSD(label + "_SD", centre, m_id_x, m_id_y, m_id_z));
    // TODO: pass pv and extract global coordinates from it!
    // m_patientSD.Put(new D3DCellSD(label+"_SD",pv,m_id_x,m_id_y,m_id_z));
    // 1. Sometimes we want to dump local, but sometimes global positioning
    // 2. The easiest way for human would be defining positioning of cells in local system.
    //    In fact now it's in local? Why Kuba had to tweak positioning for testbeam phantom?
    //    Because initial frame of phantom box is for "rotated" box;
    //    Would be nice to have for this model a function rotateX(-90) and rotateY(-90),
    //    of intuitively placed cells, then this placement pass to the actual phantomenv
    //    which frame is rotated.
    auto patientSD = m_patientSD.Get();


    G4String hcName;
    // Scoring in the centre of the cell
    // ________________________________________________________________________
    hcName = label + "_HC";
    // G4cout << "Current cell hcName: " << hcName << G4endl;
    G4int nvx(1), nvy(1), nvz(1);  // Scoring resolution: nVoxelsX, nVoxelsY, nVoxelsZ
    if (D3DCell::m_set_cell_voxelised_scorer) {
      nvx = m_cell_voxelization_x;
      nvy = m_cell_voxelization_y;
      nvz = m_cell_voxelization_z;
    }
    std::string name = GetName();
    // TEMPORARY METHOD TO GET RUN COLLECTION NAME:
    // TODO: extract this from Detector::name scope
    std::string runCollName = name.substr(0, name.find('_', 0));
    // G4cout << "[DEBUG]:: D3DCell::DefineSensitiveDetector name " << name << " runCollName " << runCollName << G4endl;
    patientSD->AddScoringVolume(runCollName, hcName, *envBox, nvx, nvy, nvz);

    // ________________________________________________________________________
    VPatient::SetSensitiveDetector(label + "LV", patientSD);  // this call G4SDManager::GetSDMpointer()->AddNewDetector(aSD);
  }
}
