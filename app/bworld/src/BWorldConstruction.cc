#include "BWorldConstruction.hh"
#include "Services.hh"
#include "G4Box.hh"
#include "Types.hh"

/**
 * @brief Constructs a BWorldConstruction object with default initialization.
 */
BWorldConstruction::BWorldConstruction(){}

    /**
 * @brief Destroys the BWorldConstruction object.
 *
 * Default destructor with no custom cleanup logic.
 */
BWorldConstruction::~BWorldConstruction(){}

/**
 * @brief Returns the singleton instance of BWorldConstruction.
 *
 * Ensures only one instance of BWorldConstruction exists, managed externally by G4GeometryManager.
 *
 * @return Pointer to the singleton BWorldConstruction instance.
 */
WorldConstruction* BWorldConstruction::GetInstance() {
    static auto instance = new BWorldConstruction(); // It's being released by G4GeometryManager
    return instance;
}

/**
 * @brief Constructs the simulation world geometry, including world, bunker, and environment volumes.
 *
 * Creates the main world volume at the configured isocenter, adds a concrete bunker wall and an inner bunker environment, and places them hierarchically. Also constructs world modules and tray detectors within the bunker environment.
 *
 * @return true Always returns true upon successful construction.
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
 * @brief Defines sensitive detectors and fields for the world and all tray detectors.
 *
 * Calls the base class implementation to set up sensitive detectors and fields, then defines sensitive detectors for each tray in the simulation.
 */
void BWorldConstruction::ConstructSDandField() {
    WorldConstruction::ConstructSDandField();
    for(auto& tray : m_trays){
    tray->DefineSensitiveDetector();
    }
}

/**
 * @brief Retrieves pointers to custom detectors associated with each tray.
 *
 * @return std::vector<VPatient*> Vector of pointers to the detectors from all tray objects.
 */
std::vector<VPatient*> BWorldConstruction::GetCustomDetectors() const {
    std::vector<VPatient*> customDetectors;
    for(const auto& tray : m_trays){
        customDetectors.push_back(tray->GetDetector());
    }
    return std::move(customDetectors);
}
