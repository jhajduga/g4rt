#include "IbaImRT.hh"
#include "G4UnionSolid.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "Services.hh"
#include "GeometryBuilder.hh"

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Construct the IbaImRT phantom wrapper.
 *
 * Creates an IbaImRT physical-volume object and initializes the base
 * IPhysicalVolume with the name "IbaImRT". The instance represents the
 * phantom geometry container used when building and placing the phantom
 * in the Geant4 world.
 */
IbaImRT::IbaImRT():IPhysicalVolume("IbaImRT"){}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Access the global IbaImRT singleton instance.
 *
 * Constructs the single IbaImRT on first use and returns a pointer to it.
 * The instance has program lifetime; initialization is thread-safe on C++11 and later.
 *
 * @return IbaImRT* Pointer to the global IbaImRT instance.
 */
IbaImRT* IbaImRT::GetInstance() {
  static IbaImRT instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
///
G4ThreeVector IbaImRT::IbaToLocalTranslation(0.0, 0.0, 0.0);
////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs and places the IbaImRT phantom geometry into the provided parent physical volume.
 *
 * Depending on the configuration key "PatientGeometry.EnviromentPatientEnvelop", this either delegates construction to the GeometryBuilder for "IbaImRT_3mf" or creates and places a PMMA phantom (simple box or composite box+tube shape) using Geant4 solids, applying the configured rotation and position.
 *
 * @param parentPV Parent Geant4 physical volume into which the phantom will be placed.
 */
void IbaImRT::Construct(G4VPhysicalVolume *parentPV) {
  
  if (ConfigSvc::GetInstance()->GetValue<std::string>("PatientGeometry", "EnviromentPatientEnvelop") != "IbaImRT_3mf") {

  G4VSolid* FullPhantom;
  auto my_rotation = new G4RotationMatrix;
  
  auto boxMaterial = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", "PMMA");
  if (ConfigSvc::GetInstance()->GetValue<std::string>("PatientGeometry", "EnviromentPatientEnvelop") == "IbaImRT_Box") {
    FullPhantom = new G4Box("smallCentreOFPhantomBox", 90.0*mm, 90.0*mm, 90.0*mm);
  }
  else if(ConfigSvc::GetInstance()->GetValue<std::string>("PatientGeometry", "EnviromentPatientEnvelop") == "IbaImRT_Full") {
      auto centreOFPhantomBox = new G4Box("smallCentreOFPhantomBox", 90.0*mm, 90.0*mm, 165.0*mm);
      auto SideOfPhantomTube = new G4Tubs("SideOfPhantomTube", 0.0*mm, 90.0*mm, 165.0*mm, 0.0*deg, 360.0*deg);
      auto FirstSideOfPhantom = new G4UnionSolid("SideOfPhantomBox", centreOFPhantomBox, SideOfPhantomTube, nullptr, G4ThreeVector(0.0*mm,-90.0*mm,0.0*mm));
      FullPhantom = new G4UnionSolid("SideOfPhantomBox", FirstSideOfPhantom, SideOfPhantomTube, nullptr, G4ThreeVector(0.0*mm,90.0*mm,0.0*mm));
      my_rotation->rotateY(90.0*deg);
      my_rotation->rotateX(90.0*deg);
    }
    else {
      //   LOGSVC_ERROR("Unknown properties of IbaImRT phantom: {}", ConfigSvc::GetInstance()->GetValue<std::string>("PatientGeometry", "EnviromentPatientEnvelop"));
    }

    auto FullPhantomLV = new G4LogicalVolume(FullPhantom, boxMaterial.get(), "IbaImRTLV");
    
    // We have to rotate it in order to right alignement of the Geant4 volumes in the world

    // Note: the position is interpreated as an isocentre nad it's injected in real Iba Frame, thus we have to translate to local frame
    SetPhysicalVolume(new G4PVPlacement(my_rotation, m_position, "IbaImRTPV", FullPhantomLV, parentPV, false, 0));
  }

  else {
    auto geometryBuilder = GeometryBuilder::GetInstance();
    geometryBuilder->Build(parentPV);

    // TODO: Filtrowanie elementów, które chcemy usunąć z IbaImRT
    // TODO: Musi zostać stworzony jakiś modół który pozwoli pobrać ścieżkę do DB i do CSV z z PhantomWorld

  }
}