//
// Created by jakc on 24.09.2020.
//

#include "MlcHD120.hh"
#include "Services.hh"
#include "G4Region.hh"
#include "G4ProductionCuts.hh"
#include "G4PVReplica.hh"
#include "G4Box.hh"
#include "G4Tubs.hh"
#include "G4LogicalVolume.hh"
#include "G4RotationMatrix.hh"
#include "G4PVPlacement.hh"
#include "G4ReflectedSolid.hh"
#include <memory>
////////////////////////////////////////////////////////////////////////////////
///
MlcHd120::MlcHd120():IPhysicalVolume("MlcHd120"), VMlc("MlcHd120"){
    // Region and default production cuts
    m_mlc_region = std::make_unique<G4Region>("MlcHd120Region");
    m_mlc_region->SetProductionCuts(new G4ProductionCuts());
    m_mlc_region->GetProductionCuts()->SetProductionCut(1.0 * cm);
}

////////////////////////////////////////////////////////////////////////////////
///
void MlcHd120::Construct(G4VPhysicalVolume *parentPV){
    G4cout << "\n[INFO]::  Construction of the " << GetName() << G4endl;

    auto W_All = Service<ConfigSvc>()->GetValue<G4MaterialSPtr>("MaterialsSvc", "tungstenAlloy1");

    CreateMlcModules(parentPV,W_All.get());
}

////////////////////////////////////////////////////////////////////////////////
///
G4VPhysicalVolume* MlcHd120::CreateMlcModules(G4VPhysicalVolume* parentPV, G4Material* material){

    // TODO
    // Reafactor, using lambda, also take into account X:Y switich, for rotation of the leafs

    /////////////////////////////////////////////////////////////////////////////
    //  Creating the MLC world.
    /////////////////////////////////////////////////////////////////////////////

    // auto zShiftInLinacWorld = parentPV->GetTranslation().getZ() + 373.75 * mm;
    auto zShiftInLinacWorld = VMlc::GetMlcZPosition(); // TO BE VALIDATED
    auto zTranslationInLinacWorld = G4ThreeVector(0.,0.,-zShiftInLinacWorld);
    auto mlcWorldPV = parentPV;
    /////////////////////////////////////////////////////////////////////////////
    //  Giving shape to the leaves located in the center of the MLC.
    /////////////////////////////////////////////////////////////////////////////

    auto centralLeafShape = CreateCentralLeafShape();
    auto centralLeafLV = new G4LogicalVolume(centralLeafShape, material, "centralLeafLV", 0, 0, 0);
    centralLeafLV->SetRegion(m_mlc_region.get());

    /////////////////////////////////////////////////////////////////////////////
    //  Giving shape to the leaves on the outer parts of the MLC.
    /////////////////////////////////////////////////////////////////////////////

    auto sideLeafShape = CreateSideLeafShape();
    auto sideLeafLV = new G4LogicalVolume(sideLeafShape, material, "sideLeafLV", 0, 0, 0);
    sideLeafLV->SetRegion(m_mlc_region.get());

    /////////////////////////////////////////////////////////////////////////////
    //  The shape of the transition leaves between wide and narrow leaves - wide version.
    /////////////////////////////////////////////////////////////////////////////
    auto transitionLeafShape1 = CreateTransitionLeafShape("SideA");
    G4VSolid* refSolidTransLeaf1 = new G4ReflectedSolid("transitionLeaf",transitionLeafShape1,G4ScaleZ3D(-1.0));
    auto transitionLeafLV1 = new G4LogicalVolume(transitionLeafShape1, material, "transitionLeafLV", 0, 0, 0);
    auto refTransitionLeafLV1 = new G4LogicalVolume(refSolidTransLeaf1, material, "transitionLeafLV", 0, 0, 0);
    transitionLeafLV1->SetRegion(m_mlc_region.get());


    auto transitionLeafShape1B = CreateTransitionLeafShape("SideB");
    G4VSolid* refSolidTransLeaf1B = new G4ReflectedSolid("transitionLeafB",transitionLeafShape1,G4ScaleZ3D(-1.0));
    auto transitionLeafLV1B = new G4LogicalVolume(transitionLeafShape1B, material, "transitionLeafLVB", 0, 0, 0);
    auto refTransitionLeafLV1B = new G4LogicalVolume(refSolidTransLeaf1B, material, "transitionLeafLVB", 0, 0, 0);
    transitionLeafLV1B->SetRegion(m_mlc_region.get());

    /////////////////////////////////////////////////////////////////////////////
    //  The shape of the transition leaves between wide and narrow leaves - narrow version.
    /////////////////////////////////////////////////////////////////////////////

    auto transitionLeafShape2 = CreateTransitionLeafShape("CentralA");
    G4VSolid* refSolidTransLeaf2 = new G4ReflectedSolid("transitionLeaf2",transitionLeafShape2,G4ScaleZ3D(-1.0));
    auto transitionLeafLV2 = new G4LogicalVolume(transitionLeafShape2, material, "transitionLeaf2LV", 0, 0, 0);
    auto refTransitionLeafLV2 = new G4LogicalVolume(refSolidTransLeaf2, material, "transitionLeaf2LV", 0, 0, 0);
    transitionLeafLV2->SetRegion(m_mlc_region.get());

    auto transitionLeafShape2B = CreateTransitionLeafShape("CentralB");
    G4VSolid* refSolidTransLeaf2B = new G4ReflectedSolid("transitionLeaf2B",transitionLeafShape2B,G4ScaleZ3D(-1.0));
    auto transitionLeafLV2B = new G4LogicalVolume(transitionLeafShape2B, material, "transitionLeaf2LVB", 0, 0, 0);
    auto refTransitionLeafLV2B = new G4LogicalVolume(refSolidTransLeaf2B, material, "transitionLeaf2LVB", 0, 0, 0);
    transitionLeafLV2B->SetRegion(m_mlc_region.get());

    /////////////////////////////////////////////////////////////////////////////
    //  The orientation of leaves in space - Version 1.
    /////////////////////////////////////////////////////////////////////////////

    G4RotationMatrix* leavesOrientation1 = new G4RotationMatrix();
    leavesOrientation1->rotateX(90.*deg);
    leavesOrientation1->rotateY(180.*deg);
    leavesOrientation1->rotateZ(0.*deg);

    /////////////////////////////////////////////////////////////////////////////
    //  The orientation of leaves in space - Version 2.
    /////////////////////////////////////////////////////////////////////////////

    G4RotationMatrix* leavesOrientation2 = new G4RotationMatrix();
    leavesOrientation2->rotateX(90.*deg);
    leavesOrientation2->rotateY(0.*deg);
    leavesOrientation2->rotateZ(180.*deg);

    /////////////////////////////////////////////////////////////////////////////
    //  The orientation of leaves in space - Version 3.
    /////////////////////////////////////////////////////////////////////////////

    G4RotationMatrix* leavesOrientation3 = new G4RotationMatrix();
    leavesOrientation3->rotateX(90.*deg);
    leavesOrientation3->rotateY(0.*deg);
    leavesOrientation3->rotateZ(0.*deg);

    /////////////////////////////////////////////////////////////////////////////
    //  The orientation of leaves in space - Version 4.
    /////////////////////////////////////////////////////////////////////////////

    G4RotationMatrix* leavesOrientation4 = new G4RotationMatrix();
    leavesOrientation4->rotateX(90.*deg);
    leavesOrientation4->rotateY(180.*deg);
    leavesOrientation4->rotateZ(180.*deg);

    /////////////////////////////////////////////////////////////////////////////
    //  The placement of leaves in space.
    /////////////////////////////////////////////////////////////////////////////

    G4LogicalVolume *leafLV;
    G4ThreeVector leafOneCentre3Vec_a( 16.*cm, -11*cm, 0.051*cm);
    G4ThreeVector leafOneCentre3Vec_b( 16.*cm, -10.493*cm, -0.051*cm);
    G4ThreeVector leafTwoCentre3Vec_a( -16.*cm, -11*cm, 0.051*cm);
    G4ThreeVector leafTwoCentre3Vec_b( -16.*cm, -10.493*cm, -0.051*cm);
    leafOneCentre3Vec_a+=zTranslationInLinacWorld;
    leafTwoCentre3Vec_a+=zTranslationInLinacWorld;
    leafOneCentre3Vec_b+=zTranslationInLinacWorld;
    leafTwoCentre3Vec_b+=zTranslationInLinacWorld;

    /////////////////////////////////////////////////////////////////////////////
    //  Entry of initial logical volume to loop.
    /////////////////////////////////////////////////////////////////////////////

    leafLV = sideLeafLV;
    G4double shiftStep_fact = 2.0;

    for (unsigned i = 0; i < 60; ++i) {

        /////////////////////////////////////////////////////////////////////////////
        //  The if/else function divides the leaves into even and odd -
        //  - mirrored leaf reflections from the top and bottom of the MLV.
        //  Even side - top.
        /////////////////////////////////////////////////////////////////////////////

        if (i%2==0) {
            // std::cout << i << std::endl;
            auto name = "LeafY1PV" + G4String(std::to_string(i));

            /////////////////////////////////////////////////////////////////////////////
            //  Creating a pair leaves on opposite sides of the MLC - corresponding leaves.
            /////////////////////////////////////////////////////////////////////////////
            G4double shiftStep;
            /////////////////////////////////////////////////////////////////////////////
            //  The leaves numbered from 0 to 12 $ from 47 to 59 belong to the side leaf class.
            //  Below we exclude rest of the leaves (13 - 46) from the side leaf class.
            /////////////////////////////////////////////////////////////////////////////
            if (i > 12 && i < 47) {
                if (i == 14){

                    /////////////////////////////////////////////////////////////////////////////
                    // Leaf 14 -  Creating a pair of transition leaves - narrow version.
                    /////////////////////////////////////////////////////////////////////////////

                    shiftStep = 4.35 * mm;
                    shiftStep*= shiftStep_fact;
                    leafLV = transitionLeafLV2;
                }
                else if (i == 46){

                    /////////////////////////////////////////////////////////////////////////////
                    // Leaf 46 -  Creating a pair of transition leaves - wide version.
                    /////////////////////////////////////////////////////////////////////////////

                    shiftStep = 3.29 * mm;
                    shiftStep*= shiftStep_fact;
                    leafLV = transitionLeafLV1B;

                }
                else if (i == 16){

                    /////////////////////////////////////////////////////////////////////////////
                    // Leaf 46 -  Creating a pair of transition leaves - wide version.
                    /////////////////////////////////////////////////////////////////////////////

                    shiftStep = 2.64 * mm;
                    shiftStep*= shiftStep_fact;
                    leafLV = centralLeafLV;
                }
                else {

                    /////////////////////////////////////////////////////////////////////////////
                    //  Creating a pair of central leaves.
                    /////////////////////////////////////////////////////////////////////////////

                    shiftStep = 2.57 * mm;
                    shiftStep*= shiftStep_fact;
                    leafLV = centralLeafLV;
                }
                leafOneCentre3Vec_a.setY(leafOneCentre3Vec_a.getY() + shiftStep);
                leafTwoCentre3Vec_a.setY(leafTwoCentre3Vec_a.getY() + shiftStep);
            } else if (i == 48){

                /////////////////////////////////////////////////////////////////////////////
                //  Creating a pair of side leaves.
                /////////////////////////////////////////////////////////////////////////////
                shiftStep = 4.98 * mm;
                shiftStep*= shiftStep_fact;
                leafLV = sideLeafLV;

                leafOneCentre3Vec_a.setY(leafOneCentre3Vec_a.getY() + shiftStep);
                leafTwoCentre3Vec_a.setY(leafTwoCentre3Vec_a.getY() + shiftStep);
            }
            else {

                /////////////////////////////////////////////////////////////////////////////
                //  Creating a pair of side leaves.
                /////////////////////////////////////////////////////////////////////////////
                shiftStep = 5.07 * mm;
                shiftStep*= shiftStep_fact;
                leafLV = sideLeafLV;

                leafOneCentre3Vec_a.setY(leafOneCentre3Vec_a.getY() + shiftStep);
                leafTwoCentre3Vec_a.setY(leafTwoCentre3Vec_a.getY() + shiftStep);
            }
            if (i == 14) {
            m_y1_leaves.push_back(
                    std::make_unique<G4PVPlacement>(leavesOrientation1, leafOneCentre3Vec_a, name+"A", leafLV, mlcWorldPV, false, i));
            m_y2_leaves.push_back(
                    std::make_unique<G4PVPlacement>(leavesOrientation3, leafTwoCentre3Vec_a, name+"B", refTransitionLeafLV2B, mlcWorldPV, false, i));
            }
            else if (i == 46) {
            m_y1_leaves.push_back(
                    std::make_unique<G4PVPlacement>(leavesOrientation1, leafOneCentre3Vec_a, name+"A", leafLV, mlcWorldPV, false, i));
            m_y2_leaves.push_back(
                    std::make_unique<G4PVPlacement>(leavesOrientation3, leafTwoCentre3Vec_a, name+"B", refTransitionLeafLV1, mlcWorldPV, false, i));
            }
            else{
            m_y1_leaves.push_back(
                    std::make_unique<G4PVPlacement>(leavesOrientation1, leafOneCentre3Vec_a, name+"A", leafLV, mlcWorldPV, false, i));
            m_y2_leaves.push_back(
                    std::make_unique<G4PVPlacement>(leavesOrientation3, leafTwoCentre3Vec_a, name+"B", leafLV, mlcWorldPV, false, i));
            }
        }

        /////////////////////////////////////////////////////////////////////////////
        //  The if/else function divides the leaves into even and odd -
        //  - mirrored leaf reflections from the top and bottom of the MLV.
        //  Odd side - bottom.
        /////////////////////////////////////////////////////////////////////////////

        else {
            // std::cout << i << std::endl;
            auto name = "LeafY1PV" + G4String(std::to_string(i));

            /////////////////////////////////////////////////////////////////////////////
            //  Creating a pair leaves on opposite sides of the MLC - corresponding leaves.
            /////////////////////////////////////////////////////////////////////////////


            G4double shiftStep;

            /////////////////////////////////////////////////////////////////////////////
            //  The leaves numbered from 0 to 12 $ from 47 to 59 belong to the side leaf class.
            //  Below we exclude rest of the leaves (13 - 46) from the side leaf class.
            /////////////////////////////////////////////////////////////////////////////

            if (i > 12 && i < 47) { // central leafes
                if (i == 13) {

                    /////////////////////////////////////////////////////////////////////////////
                    // Leaf 13 -  Creating a pair of transition leaves - wide version.
                    /////////////////////////////////////////////////////////////////////////////

                    shiftStep = 4.98 * mm;
                    shiftStep*= shiftStep_fact;
                    leafLV = transitionLeafLV1;
                }
                else  if (i == 45){

                    /////////////////////////////////////////////////////////////////////////////
                    // Leaf 45 -  Creating a pair of transition leaves - narrow version.
                    /////////////////////////////////////////////////////////////////////////////

                    shiftStep = 2.65 * mm;
                    shiftStep*= shiftStep_fact;
                    leafLV = transitionLeafLV2B;
                }
                else  if (i == 15){

                    /////////////////////////////////////////////////////////////////////////////
                    // Leaf 45 -  Creating a pair of transition leaves - narrow version.
                    /////////////////////////////////////////////////////////////////////////////

                    shiftStep = 3.27 * mm;
                    shiftStep*= shiftStep_fact;
                    leafLV = centralLeafLV;
                }
                else {

                    /////////////////////////////////////////////////////////////////////////////
                    //  Creating a pair of central leaves.
                    /////////////////////////////////////////////////////////////////////////////

                    shiftStep = 2.57 * mm;
                    shiftStep*= shiftStep_fact;
                    leafLV = centralLeafLV;
                }
                leafOneCentre3Vec_b.setY(leafOneCentre3Vec_b.getY() + shiftStep);
                leafTwoCentre3Vec_b.setY(leafTwoCentre3Vec_b.getY() + shiftStep);
            }
            else if (i == 47){
                /////////////////////////////////////////////////////////////////////////////
                //  Creating a pair of side leaves.
                /////////////////////////////////////////////////////////////////////////////

                shiftStep = 4.36 * mm;
                shiftStep*= shiftStep_fact;
                leafLV = sideLeafLV;

                leafOneCentre3Vec_b.setY(leafOneCentre3Vec_b.getY() + shiftStep);
                leafTwoCentre3Vec_b.setY(leafTwoCentre3Vec_b.getY() + shiftStep);
            }
            else{
                /////////////////////////////////////////////////////////////////////////////
                //  Creating a pair of side leaves.
                /////////////////////////////////////////////////////////////////////////////

                shiftStep = 5.07 * mm;
                shiftStep*= shiftStep_fact;
                leafLV = sideLeafLV;

                leafOneCentre3Vec_b.setY(leafOneCentre3Vec_b.getY() + shiftStep);
                leafTwoCentre3Vec_b.setY(leafTwoCentre3Vec_b.getY() + shiftStep);
            }
            if (i == 13) {
            m_y1_leaves.push_back(
                    std::make_unique<G4PVPlacement>(leavesOrientation2, leafOneCentre3Vec_b, name+"A", leafLV, mlcWorldPV, false, i));
            m_y2_leaves.push_back(
                    std::make_unique<G4PVPlacement>(leavesOrientation4, leafTwoCentre3Vec_b, name+"B", refTransitionLeafLV1, mlcWorldPV, false, i));
            }
            else if (i == 45) {
            m_y1_leaves.push_back(
                    std::make_unique<G4PVPlacement>(leavesOrientation2, leafOneCentre3Vec_b, name+"A", leafLV, mlcWorldPV, false, i));
            m_y2_leaves.push_back(
                    std::make_unique<G4PVPlacement>(leavesOrientation4, leafTwoCentre3Vec_b, name+"B", refTransitionLeafLV2B, mlcWorldPV, false, i));
            }
            else{
            m_y1_leaves.push_back(
                    std::make_unique<G4PVPlacement>(leavesOrientation2, leafOneCentre3Vec_b, name+"A", leafLV, mlcWorldPV, false, i));
            m_y2_leaves.push_back(
                    std::make_unique<G4PVPlacement>(leavesOrientation4, leafTwoCentre3Vec_b, name+"B", leafLV, mlcWorldPV, false, i));
            }
        }
    }
    mlcWorldPV->CheckOverlaps();
    return mlcWorldPV;
}

/////////////////////////////////////////////////////////////////////////////
//  Giving the shape to the central leaf.
/////////////////////////////////////////////////////////////////////////////

G4VSolid* MlcHd120::CreateCentralLeafShape() const {

    G4double leafWidth = 3.22 * mm;

    auto cylinderCentralLeaf = new G4Tubs("CentralLeafTube",
                                               m_innerRadius / 1.,
                                               m_cylinderRadius / 1.,
                                               leafWidth / 2.,
                                               0 / 1.,
                                               2 * M_PI / 1.);


    auto boxCentralLeaf = new G4Box("CentralLeafBox",
                                         m_leafLength / 2.,
                                         m_leafHeight / 2.,
                                        (2 * cm + leafWidth / 2.));


    auto boxCentralLeaf2 = new G4Box("CentralLeafBox2",
                                          ((4. * cm) + m_leafLength / 2.),
                                          m_leafHeight / 2.,
                                          leafWidth / 2.);

    auto intersectionCentralLeaf  = new G4IntersectionSolid("PrimalCentralLeaf",
                                                              cylinderCentralLeaf,
                                                              boxCentralLeaf,
                                                              0,
                                                              G4ThreeVector( 1.5*cm,0.,0.));

    auto halfDoneCentralLeaf = new G4SubtractionSolid("HalfDoneCentralLeaf",
                                                        intersectionCentralLeaf,
                                                        boxCentralLeaf2,
                                                        0,
                                                        G4ThreeVector(0., 3.5*cm, 2.52*mm));

    auto almostDoneCentralLeaf = new G4SubtractionSolid("CentralLeaf0",
                                                          halfDoneCentralLeaf,
                                                          boxCentralLeaf2,
                                                          0,
                                                          G4ThreeVector(0.,3.5*cm,-2.52*mm));

    auto endCapBox = CreateEndCapCutBox();

    auto centralLeaf = new G4SubtractionSolid("CentralLeaf",
                                                 almostDoneCentralLeaf,
                                                 endCapBox,
                                                 0,
                                                 G4ThreeVector(-10.*cm, 0., 0.));


    return centralLeaf;
}

/////////////////////////////////////////////////////////////////////////////
//  Giving the shape to the side leaf.
/////////////////////////////////////////////////////////////////////////////

G4VSolid* MlcHd120::CreateSideLeafShape() const {

    G4double leafWidth = 6.45 * mm;

    auto cylinderSideLeaf = new G4Tubs("SideLeafTube",
                                            m_innerRadius / 1.,
                                            m_cylinderRadius / 1.,
                                            leafWidth / 2.,
                                            0 / 1.,
                                            2 * M_PI / 1.);


    auto boxSideLeaf = new G4Box("SideLeafBox",
                                      m_leafLength / 2.,
                                      m_leafHeight / 2.,
                                      (2 * cm + leafWidth / 2.));


    auto boxSideLeaf2 = new G4Box("SideLeafBox",
                                       ((4. * cm) + m_leafLength / 2.),
                                       m_leafHeight / 2.,
                                       leafWidth / 2.);

    auto intersectionSideLeaf  = new G4IntersectionSolid("PrimalSideLeaf",
                                                               cylinderSideLeaf,
                                                               boxSideLeaf,
                                                               0,
                                                               G4ThreeVector(1.5*cm,0.,0.));

    auto halfDoneSideLeaf = new G4SubtractionSolid("HalfDoneSideLeaf",
                                                    intersectionSideLeaf,
                                                    boxSideLeaf2,
                                                    0,
                                                    G4ThreeVector(0., 3.5*cm, 5.05*mm));

    auto almostDoneSideLeaf = new G4SubtractionSolid("SideLeaf0",
                                                halfDoneSideLeaf ,
                                                boxSideLeaf2,
                                                0,
                                                G4ThreeVector(0., 3.5*cm, -5.05*mm));


    auto endCapBox = CreateEndCapCutBox();

    auto sideLeaf = new G4SubtractionSolid("SideLeaf",
                                            almostDoneSideLeaf,
                                            endCapBox,
                                            0,
                                            G4ThreeVector(-10.*cm, 0., 0.));
    return sideLeaf;
}

/////////////////////////////////////////////////////////////////////////////
//  Giving the shape to the transition leaf.
/////////////////////////////////////////////////////////////////////////////

G4VSolid* MlcHd120::CreateTransitionLeafShape(const std::string& type) const{

    auto createLeafShape = [&](const G4double& leafWidth, 
                            const G4ThreeVector& cut_offset_left,
                            const G4ThreeVector& cut_offset_right) -> G4VSolid* {
        auto innerEndCap = new G4Tubs("innerEndCap",
                                        m_innerRadius,
                                        m_cylinderRadius,
                                        leafWidth / 2.,
                                        0,
                                        2 * M_PI);
        auto narrowBox = new G4Box("narrowBox",
                                        m_leafLength / 2.,
                                        m_leafHeight / 2.,
                                        (2 * cm + leafWidth / 2.));
        auto wideBox = new G4Box("wideBox",
                                        ((4. * cm) + m_leafLength / 2.),
                                        m_leafHeight / 2.,
                                        leafWidth / 2.);
        auto roundedLeafShape  = new G4IntersectionSolid("roundedLeafShape",
                                                        innerEndCap,
                                                        narrowBox,
                                                        nullptr,
                                                        G4ThreeVector( 1.5 * cm,0., 0.));
        auto first_halfLeaf = new G4SubtractionSolid("first_halfLeaf",
                                                        roundedLeafShape,
                                                        wideBox,
                                                        nullptr,
                                                        cut_offset_left);
        auto second_halfLeaf = new G4SubtractionSolid("second_halfLeaf",
                                                        first_halfLeaf,
                                                        wideBox,
                                                        nullptr,
                                                        cut_offset_right);
        return new G4SubtractionSolid("transitionLeaf",
                                    second_halfLeaf,
                                    CreateEndCapCutBox(),
                                    nullptr,
                                    G4ThreeVector(-10.*cm, 0.*cm, 0.*mm));
    };

    G4double side_leaf_width = 6.1 * mm;
    G4ThreeVector side_leaf_offset_left(0., 3.5*cm, 5.05*mm);
    G4ThreeVector side_leaf_offset_right(0., 3.5*cm, -4.69*mm);

    G4double central_leaf_width = 3.57 * mm;
    G4ThreeVector central_leaf_offset_left(0., 3.5*cm, 2.52*mm);
    G4ThreeVector central_leaf_offset_right(0.*cm, 3.5*cm, -2.87*mm);
    
    if (type.compare("SideA") == 0){
        return createLeafShape(side_leaf_width, side_leaf_offset_left,side_leaf_offset_right);
    } else if (type.compare("SideB") == 0){
        return createLeafShape(side_leaf_width, side_leaf_offset_right, side_leaf_offset_left);
    } else if (type.compare("CentralA") == 0){
        return createLeafShape(central_leaf_width, central_leaf_offset_left,central_leaf_offset_right);
    } else if (type.compare("CentralB") == 0){
        return createLeafShape(central_leaf_width, central_leaf_offset_right, central_leaf_offset_left);
    }

}

////////////////////////////////////////////////////////////////////////////////
///
G4VSolid* MlcHd120::CreateEndCapCutBox() const {
    return new G4Box("EndCapCutBox", 10. * cm, 10. * cm, 10. * cm);
}

////////////////////////////////////////////////////////////////////////////////
///
G4bool MlcHd120::Update(){
    // implement me.
    return true;
}

////////////////////////////////////////////////////////////////////////////////
///
void MlcHd120::Reset(){
    // implement me.
}

////////////////////////////////////////////////////////////////////////////////
///
void MlcHd120::WriteInfo(){
    // implement me.
}

////////////////////////////////////////////////////////////////////////////////
///
void MlcHd120::SetRunConfiguration(const ControlPoint* control_point){
    auto inputType = control_point->GetFieldType();
    //thisConfig()->GetValue<std::string>("PositionningFileType");
    G4cout << "[INFO]:: MlcHd120:: the run configuration type: "<< inputType << G4endl;

    if(inputType=="CustomPlan"){
        SetCustomPositioning(control_point);
    }
    else if(inputType=="RTPlan"){
        auto beamId = int(0);         // temporary fixed; it will come from LinacRun instance
        auto controlPointId = int(0); // temporary fixed; it will come from LinacRun instance
        SetRTPlanPositioning(beamId,controlPointId);
    }
    m_control_point_id = control_point->Id();
    m_isInitialized = true; 
}

////////////////////////////////////////////////////////////////////////////////
///
void MlcHd120::SetCustomPositioning(const ControlPoint* control_point){
    const auto& mlc_a_positioning = control_point->GetMlcPositioning("Y1");
    const auto& mlc_b_positioning = control_point->GetMlcPositioning("Y2");
    for(int leaf_idx = 0; leaf_idx < mlc_a_positioning.size(); leaf_idx++){
        auto y1_translation = m_y1_leaves[leaf_idx]->GetTranslation();
        y1_translation.setX(y1_translation.getX()-mlc_a_positioning.at(leaf_idx));
        m_y1_leaves[leaf_idx]->SetTranslation(y1_translation);
        VMlc::m_leaves_x_positioning.emplace_back(y1_translation.getX());
    }
    for(int leaf_idx = 0; leaf_idx < mlc_b_positioning.size(); leaf_idx++){
        auto y2_translation = m_y2_leaves[leaf_idx]->GetTranslation();
        y2_translation.setX(y2_translation.getX()-mlc_b_positioning.at(leaf_idx));
        m_y2_leaves[leaf_idx]->SetTranslation(y2_translation);
    }
}

////////////////////////////////////////////////////////////////////////////////
///
void MlcHd120::SetRTPlanPositioning(int current_beam, int current_controlpoint){
    auto contolPoint = Service<RunSvc>()->CurrentControlPoint();
    const auto& pos1 = contolPoint->GetMlcPositioning("Y1");
    const auto& pos2 = contolPoint->GetMlcPositioning("Y1");

    if(pos1.size()!=pos2.size() || pos1.size()!=60){
        G4cout << "[DEBUG]:: MlcHd120:: posY1.size() " << pos1.size() << G4endl;
        G4cout << "[DEBUG]:: MlcHd120:: posY2.size() " << pos2.size() << G4endl;
        G4Exception("MlcHd120", "SetRTPlanPositioning", FatalErrorInArgument, "Wrong MLC positioning data retrieved!");
    }
    VMlc::m_leaves_x_positioning.clear();
    for(int i=0; i<pos1.size(); ++i){
        G4cout << "[DEBUG]:: MlcHd120:: Y1 "<<pos1.at(i)<<", Y2 "<< pos2.at(i) << G4endl;
        // overlap check: TODO this should be done in filling the positioning vectors?
        // if(pos2.at(i)-pos1.at(i)<0){
        //     G4cout << "[WARNING]:: MlcHd120:: OVERLAP Y1 "<<pos1.at(i)<<", Y2 "<< pos2.at(i) << G4endl;
        //     pos1[i] = 0;
        //     pos2[i] = 0;
        // }

        auto y1_translation = m_y1_leaves[i]->GetTranslation();
        y1_translation.setX(y1_translation.getX()+pos2.at(i) );
        m_y1_leaves[i]->SetTranslation(y1_translation);

        VMlc::m_leaves_x_positioning.emplace_back(y1_translation.getX());

        auto y2_translation = m_y2_leaves[i]->GetTranslation();
        y2_translation.setX(y2_translation.getX()+pos1.at(i) );
        m_y2_leaves[i]->SetTranslation(y2_translation);
    }
}

