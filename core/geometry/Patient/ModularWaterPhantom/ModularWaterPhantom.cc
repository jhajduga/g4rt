#include "ModularWaterPhantom.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "Services.hh"
#include "G4SubtractionSolid.hh"
#include "G4UnionSolid.hh"
#include "GeometryBuilder.hh"


////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs a ModularWaterPhantom object with the default name.
 *
 * Initializes the base IPhysicalVolume with the name "ModularWaterPhantom".
 */
ModularWaterPhantom::ModularWaterPhantom():IPhysicalVolume("ModularWaterPhantom"){}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Accesses the unique ModularWaterPhantom singleton.
 *
 * The singleton instance is created on first use and remains valid for the program lifetime.
 *
 * @return Pointer to the unique ModularWaterPhantom instance.
 */
ModularWaterPhantom* ModularWaterPhantom::GetInstance() {
  static ModularWaterPhantom instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
///

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Build the modular water phantom geometry inside the given parent physical volume.
 *
 * Constructs the phantom according to the configuration key
 * "PatientGeometry.EnviromentPatientEnvelop". When set to
 * "ModularWaterPhantom_simplified" this creates a set of six aquarium-like shells
 * and corresponding water volumes positioned around the patient isocenter (coordinates
 * read from the configuration). When set to "ModularWaterPhantom_3mf" construction
 * is delegated to GeometryBuilder::Build for a 3MF-based phantom.
 *
 * The simplified mode obtains wall and water materials from the configuration and
 * places the assemblies using the instance rotation (m_rotation).
 *
 * @param parentPV Parent Geant4 physical volume into which the phantom volumes are placed.
 */
void ModularWaterPhantom::Construct(G4VPhysicalVolume *parentPV) {

    if (ConfigSvc::GetInstance()->GetValue<std::string>("PatientGeometry", "EnviromentPatientEnvelop") == "ModularWaterPhantom_simplified") {
    // create the actual phantom
    auto boxMaterial = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", "Usr_G4AIR20C"); // PMMA
    auto waterMaterial = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", "G4_WATER");
    auto envPosX = Service<ConfigSvc>()->GetValue<double>("PatientGeometry", "PatientIsocentreX");
    auto envPosY = Service<ConfigSvc>()->GetValue<double>("PatientGeometry", "PatientIsocentreY");
    auto envPosZ = Service<ConfigSvc>()->GetValue<double>("PatientGeometry", "PatientIsocentreZ");

    std::cout << "ModularWaterPhantom::Construct called" << __FUNCTION__ << " called\n";

    auto smallInterBox = new G4Box("smallInnerBox", 125.0*mm, 25.0*mm, 200.0*mm);
    auto smallOuterBox = new G4Box("smallOuterBox", 135.0*mm, 35.0*mm, 200.0*mm);
    auto smallAquariumBox =  new G4SubtractionSolid("smallAquaBox", smallOuterBox, smallInterBox, nullptr, G4ThreeVector(0.0*mm,0.0*mm,-10.0*mm));
    auto smallWaterFillingBox = new G4Box("smallWaterFillingBox", 125.0*mm, 25.0*mm, 165.0*mm);

    auto bigInterBox = new G4Box("bigInnerBox", 125.0*mm, 125.0*mm, 200.0*mm);
    auto bigOuterBox = new G4Box("bigOuterBox", 135.0*mm, 135.0*mm, 200.0*mm);
    auto bigAquariumBox =  new G4SubtractionSolid("bigAquaBox", bigOuterBox, bigInterBox, nullptr, G4ThreeVector(0.0*mm,0.0*mm,-10.0*mm));
    auto bigWaterFillingBox = new G4Box("bigWaterFillingBox", 125.0*mm, 125.0*mm, 165.0*mm);
    
    auto smallAquaBoxLV =  new G4LogicalVolume(smallAquariumBox, boxMaterial.get(), "smallAquaBoxLV");
    auto bigAquaBoxLV =    new G4LogicalVolume(bigAquariumBox, boxMaterial.get(), "bigAquaBoxLV");
    auto smallWaterFillingBoxLV = new G4LogicalVolume(smallWaterFillingBox, waterMaterial.get(), "smallWaterFillingBoxLV");
    auto bigWaterFillingBoxLV =   new G4LogicalVolume(bigWaterFillingBox, waterMaterial.get(), "bigWaterFillingBoxLV");
    auto pv1 =         new G4PVPlacement(m_rotation, G4ThreeVector(envPosX + 150.0*mm, envPosY, envPosZ-260.0*mm), 
                                          "smallAquaBoxPV1", smallAquaBoxLV, parentPV, false, 0);
    auto pv1_filling = new G4PVPlacement(m_rotation, G4ThreeVector(envPosX + 150.0*mm, envPosY, envPosZ-235.0*mm), 
                                          "smallWaterFillingBoxPV1", smallWaterFillingBoxLV, parentPV, false, 0);
    auto pv2 =         new G4PVPlacement(m_rotation, G4ThreeVector(envPosX - 150.0*mm, envPosY, envPosZ-260.0*mm), 
                                          "smallAquaBoxPV2", smallAquaBoxLV, parentPV, false, 0);
    auto pv2_filling = new G4PVPlacement(m_rotation, G4ThreeVector(envPosX - 150.0*mm, envPosY, envPosZ-235.0*mm), 
                                          "smallWaterFillingBoxPV2", smallWaterFillingBoxLV, parentPV, false, 0);
    auto pv3 =         new G4PVPlacement(m_rotation, G4ThreeVector(envPosX + 138.0*mm, envPosY + 173.0*mm, envPosZ-260.0*mm), 
                                          "bigAquaBoxPV3",   bigAquaBoxLV,   parentPV, false, 0);
    auto pv3_filling = new G4PVPlacement(m_rotation, G4ThreeVector(envPosX + 138.0*mm, envPosY + 173.0*mm, envPosZ-235.0*mm), 
                                          "bigWaterFillingBoxPV3", bigWaterFillingBoxLV, parentPV, false, 0);
    auto pv4 =         new G4PVPlacement(m_rotation, G4ThreeVector(envPosX - 138.0*mm, envPosY + 173.0*mm, envPosZ-260.0*mm), 
                                          "bigAquaBoxPV4",   bigAquaBoxLV,   parentPV, false, 0);
    auto pv4_filling = new G4PVPlacement(m_rotation, G4ThreeVector(envPosX - 138.0*mm, envPosY + 173.0*mm, envPosZ-235.0*mm), 
                                          "bigWaterFillingBoxPV4", bigWaterFillingBoxLV, parentPV, false, 0);
    auto pv5 =         new G4PVPlacement(m_rotation, G4ThreeVector(envPosX + 138.0*mm, envPosY - 173.0*mm, envPosZ-260.0*mm), 
                                          "bigAquaBoxPV5",   bigAquaBoxLV,   parentPV, false, 0);
    auto pv5_filling = new G4PVPlacement(m_rotation, G4ThreeVector(envPosX + 138.0*mm, envPosY - 173.0*mm, envPosZ-235.0*mm), 
                                          "bigWaterFillingBoxPV5", bigWaterFillingBoxLV, parentPV, false, 0);
    auto pv6 =         new G4PVPlacement(m_rotation, G4ThreeVector(envPosX - 138.0*mm, envPosY - 173.0*mm, envPosZ-260.0*mm), 
                                          "bigAquaBoxPV6",   bigAquaBoxLV,   parentPV, false, 0);
    auto pv6_filling = new G4PVPlacement(m_rotation, G4ThreeVector(envPosX - 138.0*mm, envPosY - 173.0*mm, envPosZ-235.0*mm), 
                                          "bigWaterFillingBoxPV6", bigWaterFillingBoxLV, parentPV, false, 0);
    }

    else if (ConfigSvc::GetInstance()->GetValue<std::string>("PatientGeometry", "EnviromentPatientEnvelop") == "ModularWaterPhantom_3mf") {
      auto geometryBuilder = GeometryBuilder::GetInstance();
      geometryBuilder->Build(parentPV);
    }

}