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
 * @brief Enables or disables cell-based scoring for TLD volumes.
 *
 * @param val Set to true to enable cell scoring, false to disable.
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
 * @brief Constructs a TLD (Thermoluminescent Dosimeter) volume with specified label, center, material, and optional STL geometry.
 *
 * Initializes a TLD instance for use in Geant4 simulations, setting its label, spatial center, material name, and STL geometry file path if provided.
 */
TLD::TLD(const G4String& label, const G4ThreeVector& centre, G4String tldMediumName, G4String stlGeometryFilePath)
    : VPatient(label), m_tld_medium(tldMediumName), m_stl_geometry_file_path(stlGeometryFilePath), m_centre(centre) {

}

////////////////////////////////////////////////////////////////////////////////
// TLD destructor
/**
 * @brief Destroys the TLD instance and releases allocated resources.
 *
 * Calls the Destroy method to clean up the TLD volume and deletes the step limit object if it was allocated.
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
 * @brief Constructs and places the TLD geometry within the simulation world.
 *
 * Builds the TLD volume using either a provided STL mesh or a manual geometry (cylinder with a hemispherical cap), assigns the specified material, and places it at the configured center position within the parent world volume. Sets up a Geant4 region with production cuts and records the volume size.
 *
 * @param parentWorld The parent world volume in which the TLD is placed.
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
 * @brief Defines and attaches a sensitive detector to the TLD logical volume.
 *
 * Creates a sensitive detector for the TLD geometry, computes the bounding scoring box, determines voxelization parameters, and registers the scoring volume for dose scoring. Ensures thread safety and prevents duplicate detector creation.
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
