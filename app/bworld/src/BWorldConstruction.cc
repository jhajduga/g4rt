#include "BWorldConstruction.hh"
#include "Services.hh"
#include "G4Box.hh"
#include "Types.hh"

BWorldConstruction::BWorldConstruction(){}

    ///
BWorldConstruction::~BWorldConstruction(){}

WorldConstruction* BWorldConstruction::GetInstance() {
    static auto instance = new BWorldConstruction(); // It's being released by G4GeometryManager
    return instance;
}

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

void BWorldConstruction::ConstructSDandField() {
    WorldConstruction::ConstructSDandField();
    for(auto& tray : m_trays){
    tray->DefineSensitiveDetector();
    }
}

std::vector<VPatient*> BWorldConstruction::GetCustomDetectors() const {
    std::vector<VPatient*> customDetectors;
    for(const auto& tray : m_trays){
        customDetectors.push_back(tray->GetDetector());
    }
    return std::move(customDetectors);
}
