#include "BWorldConstruction.hh"
#include "Services.hh"
#include "G4Box.hh"
#include "Types.hh"

/**
 * @brief Default constructor.
 *
 * Constructs a BWorldConstruction instance with no custom initialization.
 * This constructor performs only trivial construction; any heavy setup is
 * performed later (e.g., in Create()). No resources are allocated here.
 */
BWorldConstruction::BWorldConstruction(){}

    /**
 * @brief Destroys the BWorldConstruction object.
 *
 * Default destructor with no custom cleanup logic.
 */
BWorldConstruction::~BWorldConstruction(){}

/**
 * @brief Return the singleton BWorldConstruction instance.
 *
 * Creates (on first call) and returns the single global BWorldConstruction used to build
 * the simulation world. The instance's lifetime is managed externally (released by
 * G4GeometryManager); callers should not delete it.
 *
 * @return WorldConstruction* Pointer to the singleton BWorldConstruction.
 */
WorldConstruction* BWorldConstruction::GetInstance() {
    static auto instance = new BWorldConstruction(); // It's being released by G4GeometryManager
    return instance;
}

/**
 * @brief Build the simulation world and its bunker environment.
 *
 * Creates the top-level world volume positioned at the configured isocentre, then
 * creates and places a concrete bunker wall and an inner bunker environment
 * (air-filled). The method places those volumes into the world, stores the
 * bunker wall and bunker-inside placements in m_bunker_wall and
 * m_bunker_inside_pv respectively, and invokes construction of world modules
 * and tray detectors inside the bunker interior.
 *
 * The method reads geometry sizes and materials from the configuration service.
 *
 * @return true Always returns true after construction.
 */
bool BWorldConstruction::Create() {
    
     // create the world box
    auto worldSize = configSvc()->GetValue<G4ThreeVector>("WorldConstruction", "WorldSize");
    auto Air = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "Usr_G4AIR20C");
    auto worldB = new G4Box("worldG", worldSize.getX(), worldSize.getY(), worldSize.getZ());
    auto worldLV = new G4LogicalVolume(worldB, Air.get(), "worldLV", 0, 0, 0);
    auto isocentre = thisConfig()->GetValue<G4ThreeVector>("Isocentre");
    SetPhysicalVolume(new G4PVPlacement(0, isocentre, "worldPV", worldLV, 0, false, 0));

    auto world_pv = GetPhysicalVolume();
    auto envSize = G4ThreeVector(4500.*mm,4500.*mm,2000.*mm);
    auto concrete = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", "BaritesConcrete");
    G4Box *bunkerWallBox = new G4Box("bunkerWallBox",750.*mm + envSize.getX(), 750.*mm + envSize.getY(),750.*mm + envSize.getZ());
    G4LogicalVolume *bunkerWallBoxLV = new G4LogicalVolume(bunkerWallBox, concrete.get(), "bunkerWallBoxLV", 0, 0, 0);
    m_bunker_wall = new G4PVPlacement(nullptr, G4ThreeVector(0., 0., 0.), "bunkerWallBoxPV", bunkerWallBoxLV, world_pv, false, 0);
    
    G4Box *bunkerEnv = new G4Box("bunkerEnvBox", envSize.x(),envSize.y(),envSize.z());
    G4LogicalVolume *bunkerEnvLV = new G4LogicalVolume(bunkerEnv, Air.get(), "bunkerEnvLV", 0, 0, 0);
    // m_bunker_inside_pv = new G4PVPlacement(nullptr, G4ThreeVector(0., 0., 0.), "bunkerEnvBoxPV", bunkerEnvLV, world_pv, false, 0);
    m_bunker_inside_pv = new G4PVPlacement(nullptr, G4ThreeVector(0., 0., 0.), "bunkerEnvBoxPV", bunkerEnvLV, m_bunker_wall, false, 0);

    ConstructWorldModules(m_bunker_inside_pv);
    ConstructTrayDetectors(m_bunker_inside_pv);
    return true;
}

/**
 * @brief Creates and places nine tray detector objects within the specified parent volume.
 *
 * Each tray is uniquely named and added to the internal tray collection for later use.
 *
 * @param parentPV The parent physical volume in which the tray detectors are placed.
 */
void BWorldConstruction::ConstructTrayDetectors(G4VPhysicalVolume *parentPV) {
    
    m_trays.push_back(new D3DTray(parentPV, "Tray001")); 
    m_trays.push_back(new D3DTray(parentPV, "Tray002")); 
    m_trays.push_back(new D3DTray(parentPV, "Tray003")); 
    m_trays.push_back(new D3DTray(parentPV, "Tray004"));
    m_trays.push_back(new D3DTray(parentPV, "Tray005")); 
    m_trays.push_back(new D3DTray(parentPV, "Tray006")); 
    m_trays.push_back(new D3DTray(parentPV, "Tray007")); 
    m_trays.push_back(new D3DTray(parentPV, "Tray008"));
    m_trays.push_back(new D3DTray(parentPV, "Tray009"));

    // m_trays.back()->Rotate(rot);
}

/**
 * @brief Set up sensitive detectors and fields for the world and all tray detectors.
 *
 * Calls the base-class implementation to initialize global sensitive detectors and fields,
 * then invokes DefineSensitiveDetector() on each tray in m_trays so their detectors are registered.
 */
void BWorldConstruction::ConstructSDandField() {
    WorldConstruction::ConstructSDandField();
    for(auto& tray : m_trays){
    tray->DefineSensitiveDetector();
    }
}

/**
 * @brief Collects pointers to the custom patient detectors provided by each tray.
 *
 * Returns a vector of non-owning pointers to the VPatient detectors retrieved from each
 * D3DTray in m_trays. The order of pointers matches the order of trays in m_trays.
 *
 * @return std::vector<VPatient*> Non-owning pointers to tray detectors.
 */
std::vector<VPatient*> BWorldConstruction::GetCustomDetectors() const {
    std::vector<VPatient*> customDetectors;
    for(const auto& tray : m_trays){
        customDetectors.push_back(tray->GetDetector());
    }
    return std::move(customDetectors);
}
