#include "TLD.hh"
#include "G4SystemOfUnits.hh"
#include "GeoSvc.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4Sphere.hh"
#include "G4UnionSolid.hh"
#include "G4ProductionCuts.hh"
#include "TLDSD.hh"
#include "ConfigSvc.hh"
#include "G4UserLimits.hh"
#include "NTupleEventAnalisys.hh"
#include "RunAnalysis.hh"
#include <vector>
#include "Services.hh"
#include "G4VisExtent.hh"  // for GetExtent()

namespace {
G4Mutex TldMutex = G4MUTEX_INITIALIZER;
}

G4double TLD::SIZE = 4.5 * mm;

G4bool TLD::m_set_tld_scorer = true;
G4bool TLD::m_set_tld_voxelised_scorer = true;

////////////////////////////////////////////////////////////////////////////////
// TLD::CellScorer
// Enable or disable cell scorer for TLD
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Enable or disable cell-based scoring for all TLD volumes.
 *
 * Sets the global flag that controls whether TLD volumes use per-cell scoring.
 *
 * @param val True to enable cell-based scoring for TLDs; false to disable it.
 */
void TLD::CellScorer(G4bool val) {
  m_set_tld_scorer = val;
  // if(!val){ // by default it's set to true
  //   Service<RunSvc>()->GetScoringTypes().erase(Scoring::Type::Cell);
  // }
}

////////////////////////////////////////////////////////////////////////////////
// TLD::CellVoxelisedScorer
// Enable or disable voxelised cell scorer for TLD
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Enables or disables voxelised cell scoring for all TLD instances.
 *
 * When disabled, removes the voxel scoring type from the run service's scoring types.
 *
 * @param val Set to true to enable voxelised scoring, false to disable.
 */
void TLD::CellVoxelisedScorer(G4bool val) {
  m_set_tld_voxelised_scorer = val;
  if (!val) {  // by default it's set to true
    Service<RunSvc>()->GetScoringTypes().erase(Scoring::Type::Voxel);
  }
}

////////////////////////////////////////////////////////////////////////////////
// TLD constructor
/**
 * @brief Initialize a TLD instance with identity, placement and geometry metadata.
 *
 * Constructs a TLD object by storing its label, nominal center position, material name,
 * and optional STL geometry file path. This constructor only initializes object state;
 * it does not create Geant4 volumes or register sensitive detectors. Call Construct()
 * to build and place the geometry in a parent volume.
 *
 * @param label Human-readable identifier for the TLD instance (also used for run/collection naming).
 * @param centre Centre position (global coordinates) used as the nominal placement point for the TLD.
 * @param tldMediumName Name of the material/medium to assign to the TLD logical volume.
 * @param stlGeometryFilePath Path to an STL geometry file to use for the TLD shape or "None" to use the built-in shape.
 */
TLD::TLD(const G4String& label, const G4ThreeVector& centre, G4String tldMediumName, G4String stlGeometryFilePath)
    : VPatient(label), m_tld_medium(tldMediumName), m_stl_geometry_file_path(stlGeometryFilePath), m_centre(centre) {

}

////////////////////////////////////////////////////////////////////////////////
// TLD destructor
/**
 * @brief Destroy the TLD and release its resources.
 *
 * Calls Destroy() to clean up geometry and placement created for this TLD instance,
 * and deletes the optional step-limit object if one was allocated (m_step_limit).
 */
TLD::~TLD() {
  Destroy();
  if (m_step_limit) delete m_step_limit;
}

/**
 * @brief Sets the integer identifiers for the TLD instance along the x, y, and z axes.
 *
 * @param x Identifier for the x-axis.
 * @param y Identifier for the y-axis.
 * @param z Identifier for the z-axis.
 */
void TLD::SetIDs(G4int x, G4int y, G4int z) {
  m_id_x = x;
  m_id_y = y;
  m_id_z = z;
}

////////////////////////////////////////////////////////////////////////////////
// Write info (TODO)
/**
 * @brief Placeholder for outputting TLD cell information.
 *
 * Currently logs a message indicating that detailed information output is not implemented.
 */
void TLD::WriteInfo() { INFO_GEO("The Dose3D cell {} info: Implement me.", GetName()); }

////////////////////////////////////////////////////////////////////////////////
// Destroy volumes and clean up
/**
 * @brief Logs the destruction of the TLD volume.
 *
 * This method is intended to handle cleanup of the TLD's physical volume, but currently only logs the destruction event. Actual deletion of the physical volume is not performed.
 */
void TLD::Destroy() {
  INFO_GEO("Destroying the TLD volume.");
  // auto phantomVolume = GetPhysicalVolume();
  // if (phantomVolume) {
  //   delete phantomVolume;
  //   SetPhysicalVolume(nullptr);
  // }
}

////////////////////////////////////////////////////////////////////////////////
// Set number of voxels along an axis
/**
 * @brief Sets the number of voxels along a specified axis for TLD voxelization.
 *
 * @param axis The axis to set ('x', 'y', or 'z', case-insensitive).
 * @param nv The number of voxels to assign along the specified axis.
 */
void TLD::SetNVoxels(char axis, int nv) {
  switch (std::tolower(axis)) {
    case 'x':
      m_tld_voxelization_x = nv;
      break;
    case 'y':
      m_tld_voxelization_y = nv;
      break;
    case 'z':
      m_tld_voxelization_z = nv;
      break;
  }
}

////////////////////////////////////////////////////////////////////////////////
// Construct TLD geometry and place in the world
/**
 * @brief Construct and place the TLD volume inside the given parent world.
 *
 * Builds the TLD logical volume from either a provided STL mesh (cleaning a possible UTF‑8 BOM and loading a tessellated mesh) or a fallback manual geometry (solid cylinder with an upper hemispherical cap). The constructed volume is placed at the TLD's configured global center (transformed to the instance local frame), a Geant4 region with production cuts (0.1 mm) is created and assigned to the logical volume, and the TLD's volume is recorded.
 *
 * The function has the following observable effects:
 * - If m_stl_geometry_file_path != "None", reads and sanitizes the STL file, writes a cleaned temporary STL, and loads it via CADMesh to create the solid.
 * - Otherwise, creates a composite solid (tube + spherical cap) for the TLD shape.
 * - Updates m_global_centre and m_centre (global→local transform), creates and places a G4PVPlacement for the volume under parentWorld, and assigns a G4Region with 0.1 mm production cuts to the logical volume.
 * - Records the TLD volume as the product of the configured size components.
 *
 * @param parentWorld The parent physical volume in which the TLD physical volume is placed.
 */
void TLD::Construct(G4VPhysicalVolume* parentWorld) {
  // Retrieve name and material
  // std::cout << "[INFO]:: TLD construction... " << std::endl;
  auto label = GetName();
  auto material = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", m_tld_medium);

  // create a cell box filled with LiF:Mg,Ti, with given side dimensions
  // auto tldBox = new G4Box(label+"Box", size.getX() / 2., size.getY() / 2., size.getZ() / 2.);
  // G4double innerRadius = 0.0;  // Solid cylinder
  // Define cylinder size (X, Y = SIZE, Z = 0.9 mm)
  G4ThreeVector size(TLD::SIZE, TLD::SIZE, 0.9 * mm);
  G4LogicalVolume* tldLV = nullptr;

  //===================================================================
  // Load STL geometry if provided
  //===================================================================
  if (m_stl_geometry_file_path != "None") {
    std::string path = std::string(PROJECT_DATA_PATH) + "/" + m_stl_geometry_file_path;

    // Preprocess STL to remove potential UTF-8 BOM or invalid header bytes
    std::ifstream fin(path, std::ios::binary);
    std::string content((std::istreambuf_iterator<char>(fin)), {});
    fin.close();
    // Remove UTF-8 BOM if present
    const std::string bom = "ï»¿";
    if (content.rfind(bom, 0) == 0) {
      content.erase(0, bom.size());
    }
    // Write to temporary file
    std::string tmpPath = path + ".clean.stl";
    std::ofstream fout(tmpPath, std::ios::binary);
    fout << content;
    fout.close();
    // Load cleaned STL
    m_stlMesh = CADMesh::TessellatedMesh::FromSTL(tmpPath);
    G4VSolid* solidMesh = m_stlMesh->GetSolid();
    tldLV = new G4LogicalVolume(solidMesh, material.get(), label + "LV");
  } else {
    //=================================================================
    // Manual geometry: cylinder + spherical cap
    //=================================================================
    G4double innerR = 0.0;
    G4double outerR = size.x() / 2.0;
    G4double halfH = size.z() / 2.0;
    G4double startAng = 0.0;
    G4double spanAng = 360.0 * deg;

    // Cylinder
    G4VSolid* tube = new G4Tubs(label + "Tube", innerR, outerR, halfH, startAng, spanAng);

    // Spherical cap (upper hemisphere)
    G4double thetaMin = 0.0 * deg;                                  // From the top
    G4double thetaMax = 90.0 * deg;                                 // Only keep the upper half of the sphere
    G4VSolid* cap = new G4Sphere(label + "SphereCap", 0.0, outerR,  // Inner and outer radii
                                 0.0, 360.0 * deg,                  // Full azimuthal range
                                 thetaMin, thetaMax);               // Limit to a spherical slice

    // Combine tube and cap
    G4ThreeVector capOffset(0, 0, halfH);

    // Merge the tube and sphere into a single shape
    G4VSolid* combinedSolid = new G4UnionSolid(label + "CombinedSolid", tube, cap, nullptr, capOffset);
    tldLV = new G4LogicalVolume(combinedSolid, material.get(), label + "LV");

    // the placement of phantom center in the gantry (global) coordinate system that is managed by PatientGeometry class
    // here we locate the phantom box in the center of envelope box created in PatientGeometry:
  }
  m_global_centre = m_centre;

  // Compute global centre and place volume
  m_centre = svc::transformPosition(m_global_centre, this, svc::Transform::GlobalToLocal);
  SetPhysicalVolume(new G4PVPlacement(nullptr, m_centre, label + "PV", tldLV, parentWorld, false, 0));

  auto regVol = new G4Region(label + "_G4RegionCuts");
  auto cuts = new G4ProductionCuts();
  cuts->SetProductionCut(0.1 * mm);
  regVol->SetProductionCuts(cuts);
  tldLV->SetRegion(regVol);
  regVol->AddRootLogicalVolume(tldLV);

  // Record volume
  // SetVolume(tldLV->GetSolid()->GetCubicVolume());
  SetVolume(size.getX() * size.getY() * size.getZ());
}

////////////////////////////////////////////////////////////////////////////////
// Update (placeholder)
/**
 * @brief Updates the TLD instance.
 *
 * This method currently performs no operations and always returns true.
 *
 * @return G4bool Always returns true.
 */
G4bool TLD::Update() { return true; }

////////////////////////////////////////////////////////////////////////////////
// Check if sensitive detector is voxelised
/**
 * @brief Determines if the specified run collection uses voxelised scoring for this TLD volume.
 *
 * @param run_collection Name of the run collection to check.
 * @return true if the run collection is associated with a voxelised scoring volume; false otherwise.
 */
bool TLD::IsRunCollectionScoringVolumeVoxelised(const G4String& run_collection) const { return GetSD()->GetRunCollectionReferenceScoringVolume(run_collection, true) != nullptr; }

////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
// Define sensitive detector and scoring volume
/**
 * @brief Create and attach the TLD sensitive detector (SD) for dose scoring.
 *
 * Creates a TLDSD instance for this TLD, computes an axis-aligned scoring box that bounds
 * the TLD solid, and registers that box as a scoring volume with the detector. The SD is
 * configured with voxelization parameters (1×1×1 by default or the instance voxel counts
 * when voxelised scoring is enabled via TLD::m_set_tld_voxelised_scorer). The run collection
 * name is derived from the TLD label as the substring before the first underscore; the hit
 * collection name is the TLD label plus "_HC".
 *
 * Side effects:
 * - Allocates and stores a TLDSD object in m_patientSD.
 * - Calls AddScoringVolume on the TLDSD to register the scoring box and voxel grid.
 * - Attaches the SD to the TLD logical volume via VPatient::SetSensitiveDetector.
 *
 * Thread-safety and idempotence:
 * - The operation is guarded by TldMutex and will create the SD only once for this instance;
 *   subsequent calls are no-ops.
 *
 * Notes:
 * - The SD centre is positioned at the geometric centre of the solid's extent offset by the
 *   TLD's current centre.
 * - Debug output includes the TLD name, derived run-collection name, global centre, and voxel counts.
 */
void TLD::DefineSensitiveDetector() {
  G4AutoLock lock(&TldMutex);
  if (!m_patientSD.Get()) {
    // Retrieve logical volume and its solid
    auto pv = GetPhysicalVolume();
    auto lv = pv->GetLogicalVolume();
    G4VSolid* solid = lv->GetSolid();

    // Compute axis-aligned extents of the solid
    G4VisExtent vExtent = solid->GetExtent();
    G4double xmin = vExtent.GetXmin();
    G4double xmax = vExtent.GetXmax();
    G4double ymin = vExtent.GetYmin();
    G4double ymax = vExtent.GetYmax();
    G4double zmin = vExtent.GetZmin();
    G4double zmax = vExtent.GetZmax();

    // Half-sizes for scoring box
    G4double halfX = 0.5 * (xmax - xmin);
    G4double halfY = 0.5 * (ymax - ymin);
    G4double halfZ = 0.5 * (zmax - zmin);

    // Build scoring box that wraps the solid
    auto envBox = G4Box(GetName() + "ScoringBox", halfX, halfY, halfZ);
    // Adjust SD centre to shape extents centre
    G4double offsetX = 0.5 * (xmin + xmax);
    G4double offsetY = 0.5 * (ymin + ymax);
    G4double offsetZ = 0.5 * (zmin + zmax);
    G4ThreeVector sdCentre = m_centre + G4ThreeVector(offsetX, offsetY, offsetZ);

    // Create and register the sensitive detector at shape centre
    auto label = GetName();
    m_patientSD.Put(new TLDSD(label + "_SD", sdCentre, m_id_x, m_id_y, m_id_z));
    auto patientSD = m_patientSD.Get();


    // Determine voxelization grid
    G4int nvx = 1, nvy = 1, nvz = 1;
    if (TLD::m_set_tld_voxelised_scorer) {
      nvx = m_tld_voxelization_x;
      nvy = m_tld_voxelization_y;
      nvz = m_tld_voxelization_z;
    }

    // Derive run collection and hit collection names
    std::string runCollName = label.substr(0, label.find('_'));
    std::string hcName = label + "_HC";

    G4cout << "[DEBUG] TLD::DefineSensitiveDetector name=" << label << " runColl=" << runCollName << " centre=" << m_global_centre << " voxels=" << nvx << "," << nvy << "," << nvz
           << G4endl;

    // Add the scoring volume
    patientSD->AddScoringVolume(runCollName, hcName, envBox, nvx, nvy, nvz);

    // Attach the SD to the logical volume
    VPatient::SetSensitiveDetector(label + "LV", patientSD);
  }
}
