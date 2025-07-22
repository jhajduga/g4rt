#include "TLDWorldConstruction.hh"
#include "Services.hh"
#include "G4Box.hh"
#include "Types.hh"

/**
 * @brief Returns the singleton instance of TLDWorldConstruction.
 *
 * Provides access to the single instance of the world construction class, which is managed externally by the geometry manager.
 *
 * @return Pointer to the singleton TLDWorldConstruction instance.
 */
WorldConstruction* TLDWorldConstruction::GetInstance() {
    static auto instance = new TLDWorldConstruction(); // It's being released by G4GeometryManager
    return instance;
}

/**
 * @brief Constructs and initializes the simulation world volume and its main components.
 *
 * Retrieves world size and air material from configuration, creates the world geometry, and places it at the origin. After setting the world physical volume, it constructs additional world modules and the TLD tray within the world.
 *
 * @return true Always returns true upon successful creation.
 */
bool TLDWorldConstruction::Create() {
    
     // create the world box
    auto worldSize = configSvc()->GetValue<G4ThreeVector>("WorldConstruction", "WorldSize");
    auto Air = configSvc()->GetValue<G4MaterialSPtr>("MaterialsSvc", "Usr_G4AIR20C");
    auto worldB = new G4Box("worldG", worldSize.getX(), worldSize.getY(), worldSize.getZ());
    auto worldLV = new G4LogicalVolume(worldB, Air.get(), "worldLV", 0, 0, 0);
    auto centre = G4ThreeVector(0, 0, 0);
    SetPhysicalVolume(new G4PVPlacement(0, centre, "worldPV", worldLV, 0, false, 0));

    auto world_pv = GetPhysicalVolume();
    ConstructWorldModules(world_pv);
    ConstructTLDTray(world_pv);
    return true;
}

/**
 * @brief Creates and attaches a TLD tray as a child of the specified physical volume.
 *
 * Instantiates a `TLDTray` object with the given parent physical volume and stores it for later use.
 *
 * @param parentPV The parent physical volume to which the TLD tray will be attached.
 */
void TLDWorldConstruction::ConstructTLDTray(G4VPhysicalVolume *parentPV) {
    m_tld_tray = new TLDTray(parentPV, "TLDTray");
}

/**
 * @brief Sets up sensitive detectors and fields for the simulation world.
 *
 * Calls the base class implementation to construct sensitive detectors and fields, then defines a sensitive detector for the TLD tray component.
 */
void TLDWorldConstruction::ConstructSDandField() {
    WorldConstruction::ConstructSDandField();
    m_tld_tray->DefineSensitiveDetector();
}
