#include "gtest/gtest.h"
#include "PatientTest.hh"
#include "WaterPhantom.hh"

// [WIP] Test Water Phantom
TEST_F(PatientTest, WaterPhantom){
    auto toml_file = m_projectPath + "/WaterPhantom/test/water_phantom_1x1x1_test.toml";
    SvcSetup(toml_file);
    auto world = WorldSetup();
    auto patient = new WaterPhantom();
    patient->SetTomlConfigFile(toml_file);
    patient->Construct(world);
    std::string medium = patient->GetPhysicalVolume()->GetLogicalVolume()->GetMaterial()->GetName();
    EXPECT_EQ(medium,"G4_WATER");
    EXPECT_TRUE(true);
    // TODO: 
    // 1. Verify if the density of the phantom is the one of the water at 20 degree
    // 2. If the Width is more than 40 cm
    // 3. If the Depth is more than 40 cm
}

// [WIP] Test water phantom Sensitive Detector Scoring
TEST_F(PatientTest, WaterPhantomScoring){
     auto toml_file = m_projectPath + "/WaterPhantom/test/water_phantom_1x1x1_test.toml";
     SvcSetup(toml_file);
     auto world = WorldSetup();
     auto patient = new WaterPhantom();
     patient->SetTomlConfigFile(toml_file);
     patient->Construct(world); 
     patient->ConstructFullVolumeScoring("FullVolumeWaterTank");
    // TODO:: friend class error :/  // patient->ConstructFullVolumeScoring();
    // TODO: 
    // 1. Verify if the density of the phantom is the one of the water at 20 degree
    // 2. If the Width is more than 40 cm
    // 3. If the Depth is more than 40 cm
    EXPECT_TRUE(true);
}


