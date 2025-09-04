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
 * @brief Build the simulation world volume and initialize its components.
 *
 * Creates the world geometry from configuration (WorldConstruction::WorldSize)
 * using the configured air material (MaterialsSvc::Usr_G4AIR20C), places the
 * world physical volume at the origin, stores it via SetPhysicalVolume(), and
 * then constructs additional world modules and the TLD tray inside the world.
 *
 * @return true on successful construction.
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
 * @brief Create and attach a TLD tray under the given parent physical volume.
 *
 * Instantiates a TLDTray positioned as a child of parentPV and stores the resulting
 * object in the instance member m_tld_tray for later use (for example, to define
 * its sensitive detector).
 *
 * @param parentPV Parent physical volume to attach the TLD tray to. Must be a valid
 *                 G4VPhysicalVolume pointer.
 */
void TLDWorldConstruction::ConstructTLDTray(G4VPhysicalVolume *parentPV) {
    m_tld_tray = new TLDTray(parentPV, "TLDTray");
}

/**
 * @brief Configure sensitive detectors and fields for the simulation world.
 *
 * Calls the base class implementation to construct standard sensitive detectors and fields,
 * then defines the sensitive detector for the TLD tray member (m_tld_tray).
 */
void TLDWorldConstruction::ConstructSDandField() {
    WorldConstruction::ConstructSDandField();
    m_tld_tray->DefineSensitiveDetector();
}
