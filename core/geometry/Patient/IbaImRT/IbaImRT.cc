#include "IbaImRT.hh"
#include "G4UnionSolid.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "Services.hh"
#include "GeometryBuilder.hh"

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs an IbaImRT phantom geometry object.
 *
 * Initializes the IbaImRT instance as a named physical volume for use in Geant4 simulations.
 */
IbaImRT::IbaImRT():IPhysicalVolume("IbaImRT"){}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Returns the singleton instance of the IbaImRT class.
 *
 * Ensures that only one instance of IbaImRT exists throughout the application.
 *
 * @return Pointer to the single IbaImRT instance.
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
 * @brief Constructs and places the IbaImRT phantom geometry within the provided parent physical volume.
 *
 * Depending on the configuration parameter "PatientGeometry.EnviromentPatientEnvelop", this method either builds a box or composite phantom geometry using Geant4 solids and places it in the simulation, or delegates construction to the GeometryBuilder for "IbaImRT_3mf" geometries.
 *
 * @param parentPV The parent Geant4 physical volume in which the phantom geometry will be placed.
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