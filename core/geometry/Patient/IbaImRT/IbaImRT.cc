#include "IbaImRT.hh"
#include "G4UnionSolid.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "Services.hh"

////////////////////////////////////////////////////////////////////////////////
///
IbaImRT::IbaImRT():IPhysicalVolume("IbaImRT"){}

////////////////////////////////////////////////////////////////////////////////
///
IbaImRT* IbaImRT::GetInstance() {
  static IbaImRT instance;
  return &instance;
}

////////////////////////////////////////////////////////////////////////////////
///
void IbaImRT::Construct(G4VPhysicalVolume *parentPV) {
    auto boxMaterial = ConfigSvc::GetInstance()->GetValue<G4MaterialSPtr>("MaterialsSvc", "PMMA");

    auto centreOFPhantomBox = new G4Box("smallCentreOFPhantomBox", 90.0*mm, 90.0*mm, 165.0*mm);
    auto SideOfPhantomTube = new G4Tubs("SideOfPhantomTube", 0.0*mm, 90.0*mm, 165.0*mm, 0.0*deg, 360.0*deg);
    auto FirstSideOfPhantom = new G4UnionSolid("SideOfPhantomBox", centreOFPhantomBox, SideOfPhantomTube, nullptr, G4ThreeVector(0.0*mm,-90.0*mm,0.0*mm));
    auto FullPhantom = new G4UnionSolid("SideOfPhantomBox", FirstSideOfPhantom, SideOfPhantomTube, nullptr, G4ThreeVector(0.0*mm,90.0*mm,0.0*mm));

    auto FullPhantomLV = new G4LogicalVolume(FullPhantom, boxMaterial.get(), "IbaImRTLV");

    // We have to rotate it in order to right alignement in the world
    auto my_rotation = new G4RotationMatrix;
    my_rotation->rotateY(90.0*deg);
    my_rotation->rotateX(90.0*deg);

    SetPhysicalVolume(new G4PVPlacement(my_rotation, m_position, "IbaImRTPV", FullPhantomLV, parentPV, false, 0));
}