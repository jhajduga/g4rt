
#ifndef PGEO_TEST_H
#define PGEO_TEST_H

#include <memory>
#include "Services.hh"
#include "G4NistManager.hh"
#include "G4PVPlacement.hh"
#include "G4Box.hh"
#include "WorldConstruction.hh"

class PatientTest : public ::testing::Test {
protected:
    void SvcSetup(const std::string& toml_file) {
        auto configSvc = Service<ConfigSvc>();  // initialize ConfigSvc for TOML parsing
        auto runSvc = Service<RunSvc>();        // get RunSvc for general App run configuration
        runSvc->AppMode(OperationalMode::BuildGeometry);
        configSvc->ParseTomlFile(toml_file);
        configSvc->PrintTomlConfig();
        auto world = Service<GeoSvc>()->World();
        runSvc->Initialize(world);
    }
    G4VPhysicalVolume* WorldSetup() {
        auto testWorldSize = G4ThreeVector(200.0*cm,200.0*cm,200.0*cm);
        auto usrAir = G4NistManager::Instance()->FindOrBuildMaterial("G4_AIR");
        auto testWorldBox = new G4Box("worldG", testWorldSize.getX(), testWorldSize.getY(), testWorldSize.getZ());
        auto testWorldLV = new G4LogicalVolume(testWorldBox, usrAir, "worldL", 0, 0, 0);
        auto isocentre = G4ThreeVector(0.0,0.0,0.0);
        m_testWorld = new G4PVPlacement(0, isocentre, "worldPV", testWorldLV, 0, false, 0);
        return m_testWorld;
    }

    std::string m_projectPath = std::string(PROJECT_LOCATION_PATH)+"/core/geometry/Patient";
    G4PVPlacement* m_testWorld; // TODO make std::unique
};

#endif // PGEO_TEST_H