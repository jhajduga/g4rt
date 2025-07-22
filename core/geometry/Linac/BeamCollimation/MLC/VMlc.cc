#include "VMlc.hh"
#include "Services.hh"
#include "ControlPoint.hh"
#include "G4PrimaryVertex.hh"

G4ThreeVector VMlc::m_isocentre = G4ThreeVector();

/**
 * @brief Constructs a VMlc instance and initializes the isocentre position.
 *
 * Retrieves the isocentre vector from the configuration service and registers the instance with the run service.
 */
VMlc::VMlc(const std::string& name){
    m_isocentre = Service<ConfigSvc>()->GetValue<G4ThreeVector>("WorldConstruction", "Isocentre");
    AcceptRunVisitor(Service<RunSvc>());
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Registers this MLC component with the provided run service.
 *
 * Associates the VMlc instance as a run component in the simulation framework.
 */
void VMlc::AcceptRunVisitor(RunSvc *visitor){
    visitor->RegisterRunComponent(this);
}

////////////////////////////////////////////////////////////////////////////////
/**
 * @brief Projects a 3D point onto the current MLC mask plane along the line to the radiation source.
 *
 * Calculates the intersection point of a line from the given position to the source origin with the mask plane defined by the current control point's rotation and geometry.
 *
 * @param position The 3D position to be projected onto the mask plane.
 * @return G4ThreeVector The intersection point on the mask plane, rounded to 8 decimal places.
 */
G4ThreeVector VMlc::GetPositionInMaskPlane(const G4ThreeVector& position){
    // Build the plane equation
    auto cp = Service<RunSvc>()->CurrentControlPoint();
    auto rotation = cp->GetRotation();
    auto sid = Service<ConfigSvc>()->GetValue<G4double>("LinacGeometry", "SID") * mm;
    auto orign = *rotation * G4ThreeVector(0,0,-sid);
    auto normalVector = *rotation * G4ThreeVector(0,0,1);
    auto maskPoint = cp->GetPlanMaskPoints().at(0);
    // 
    auto plane_normal_x = normalVector.getX();
    auto plane_normal_y = normalVector.getY();
    auto plane_normal_z = normalVector.getZ();
    // 

    auto point_on_mask_x = maskPoint.getX();
    auto point_on_mask_y = maskPoint.getY();
    auto point_on_mask_z = maskPoint.getZ();
    // 
    auto voxcel_to_origin_x = orign.getX() - position.getX();
    auto voxcel_to_origin_y = orign.getY() - position.getY();
    auto voxcel_to_origin_z = orign.getZ() - position.getZ();

    auto voxel_centre_x = position.getX();
    auto voxel_centre_y = position.getY();
    auto voxel_centre_z = position.getZ();

    G4double t = ((plane_normal_x*point_on_mask_x + plane_normal_y*point_on_mask_y + plane_normal_z*point_on_mask_z) -
                (plane_normal_x*voxel_centre_x + plane_normal_y*voxel_centre_y + plane_normal_z*voxel_centre_z)) / 
                (plane_normal_x*voxcel_to_origin_x + plane_normal_y*voxcel_to_origin_y + plane_normal_z*voxcel_to_origin_z);


    // Find the crosssection of the line from voxel centre to origin laying the plane:
    G4double cp_vox_x = voxel_centre_x + (voxcel_to_origin_x) * t;
    G4double cp_vox_y = voxel_centre_y + (voxcel_to_origin_y) * t;
    G4double cp_vox_z = voxel_centre_z + (voxcel_to_origin_z) * t;
    return svc::round_with_prec(G4ThreeVector(cp_vox_x,cp_vox_y,cp_vox_z),8);
}

/**
 * @brief Projects the primary particle's trajectory onto the isocentre plane.
 *
 * Calculates the intersection point of the primary particle's momentum vector, starting from the given vertex position, with the plane at the isocentre z-coordinate.
 *
 * @param vrtx Pointer to the primary vertex containing position and primary particle information.
 * @return G4ThreeVector The intersection point on the isocentre plane.
 */
G4ThreeVector VMlc::GetPositionInMaskPlane(const G4PrimaryVertex* vrtx){
    auto position = vrtx->GetPosition();
    auto direction = vrtx->GetPrimary()->GetMomentum().unit();
    G4double z = m_isocentre.z();
    G4double deltaZ = z - position.getZ();
    G4double zRatio = deltaZ / direction.getZ(); 
    G4double x = position.getX() + zRatio * direction.getX(); // x + deltaX;
    G4double y = position.getY() + zRatio * direction.getY(); // y + deltaY;
    return G4ThreeVector(x, y, z);
}

/**
 * @brief Checks whether the MLC is initialized for the specified control point.
 *
 * Returns true if the internal control point ID is valid and matches the provided control point's ID; otherwise, returns false.
 */
bool VMlc::Initialized(const ControlPoint* control_point) const { 
    if(m_control_point_id<0) return false;
    if(m_control_point_id != control_point->Id()) return false;
    return true; 
}

/**
 * @brief Calculates the centroid of the open region in the MLC mask.
 *
 * Determines the geometric center (x, y) of all open leaf positions by averaging the coordinates of leaves on both Y1 and Y2 sides where the leaf gap is nonzero. The z-coordinate is taken from the first leaf position.
 *
 * @return G4ThreeVector The centroid position of the open MLC mask region.
 */
G4ThreeVector VMlc::GetMaskCentre() const{
    auto mlc_y1 = GetMlcPositioning("Y1");
    auto mlc_y2 = GetMlcPositioning("Y2");
    std::vector<std::pair<double, double>> open_positions;
    for (int i=0; i<mlc_y1.size();i++){
        // G4cout << mlc_y1.at(i) << " | " << mlc_y2.at(i) << " >> " << mlc_y1.at(i) - mlc_y2.at(i) << G4endl;
        if(mlc_y1.at(i) - mlc_y2.at(i) != G4ThreeVector()){
            open_positions.emplace_back(std::make_pair(mlc_y1.at(i).getX(),mlc_y1.at(i).getY()));
            open_positions.emplace_back(std::make_pair(mlc_y2.at(i).getX(),mlc_y2.at(i).getY()));
        }
    }

    // calculate centroid position:
    double sumX = 0.0, sumY = 0.0;
    for (const auto& point : open_positions) {
        sumX += point.first;  // x-coordinate
        sumY += point.second; // y-coordinate
    }

    double centerX = sumX / open_positions.size();
    double centerY = sumY / open_positions.size();

    return G4ThreeVector(centerX,centerY,mlc_y1.at(0).getZ());
}


/**
 * @brief Returns the 3D positions of MLC leaves for the specified side.
 *
 * Retrieves the positions of the multi-leaf collimator (MLC) leaves on the given side ("Y1" or "Y2") as a vector of 3D coordinates. If the internal x-positions are uninitialized or if the number of x-positions does not match the number of y-positions from the current control point, an empty vector is returned. The y-coordinates are negated to ensure correct orientation.
 *
 * @param side The MLC side ("Y1" or "Y2") for which to retrieve leaf positions.
 * @return std::vector<G4ThreeVector> Vector of 3D positions for the specified MLC side's leaves, or an empty vector if data is uninitialized or mismatched.
 */
std::vector<G4ThreeVector> VMlc::GetMlcPositioning(const std::string& side) const{
    // std::cout << "VMlc::GetMlcPositioning for " << side << std::endl;
    std::vector<G4ThreeVector> mlc_positioning;
    if(m_leaves_x_positioning.empty()){ // not initialized
        std::cout << "VMlc::Returning empty container for not initialized MLC!" << std::endl;   
        return mlc_positioning;
    }

    auto z = GetMlcZPosition();
    auto mlc_y_positioning = Service<RunSvc>()->CurrentControlPoint()->GetMlcPositioning(side);
    if(mlc_y_positioning.size() != m_leaves_x_positioning.size()){
        std::cout << "VMlc::Returning empty container for mismatched sizes!" << std::endl;   
        std::cout << "VMlc::mlc_x_leafs size = " << m_leaves_x_positioning.size() << std::endl;   
        std::cout << "VMlc::mlc_y_positioning size = " << mlc_y_positioning.size() << std::endl;   
        return mlc_positioning;
    }

    for(size_t i=0; i<m_leaves_x_positioning.size(); i++){
        auto x = m_leaves_x_positioning.at(i);
        auto y = - mlc_y_positioning.at(i); // For propouse proper reading from custom dat and properly oriented this (MLC lew right side orientation.) we are using -1 here. 
        mlc_positioning.push_back(G4ThreeVector(x,y,z));
    }
    return std::move(mlc_positioning);
}

/**
 * @brief Calculates the Z-coordinate of the MLC in the simulation geometry.
 *
 * Computes the MLC's Z-position based on the isocentre position, the source-to-isocentre distance (SID), and a fixed offset.
 *
 * @return G4double The computed Z-coordinate of the MLC.
 */
G4double VMlc::GetMlcZPosition() {
    auto isocentre = Service<ConfigSvc>()->GetValue<G4ThreeVector>("WorldConstruction", "Isocentre");
    auto sid = Service<ConfigSvc>()->GetValue<G4double>("LinacGeometry", "SID");
    return isocentre.z() - sid + 373.75; 
}



