#include "TLDWorldConstruction.hh"
#include "Services.hh"
#include "G4Box.hh"
#include "Types.hh"

WorldConstruction* TLDWorldConstruction::GetInstance() {
    static auto instance = new TLDWorldConstruction(); // It's being released by G4GeometryManager
    return instance;
}

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

void TLDWorldConstruction::ConstructTLDTray(G4VPhysicalVolume *parentPV) {
    m_tld_tray = new TLDTray(parentPV, "TLDTray");
}

void TLDWorldConstruction::ConstructSDandField() {
    WorldConstruction::ConstructSDandField();
    m_tld_tray->DefineSensitiveDetector();
}
