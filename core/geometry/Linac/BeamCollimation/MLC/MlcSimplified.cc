#include <vector>
#include <utility>
#include "Services.hh"
#include "BeamCollimation.hh"
#include "MlcSimplified.hh"
#include "ControlPoint.hh"

/**
 * @brief Constructs a new MlcSimplified object.
 *
 * Initializes the MlcSimplified instance by calling the base class constructor
 * with the identifier "Simplified", setting up the configuration context for a 
 * simplified multi-leaf collimator (MLC) used in radiation therapy.
 */
MlcSimplified::MlcSimplified() : VMlc("Simplified"){
    //m_control_point = Service<RunSvc>()->CurrentControlPoint();
    //m_fieldShape = m_control_point->GetFieldType();
};

/**
 * @brief Configures the simplified MLC based on control point parameters.
 *
 * This method updates the MLC configuration using the data from the given control point.
 * It sets the control point identifier and determines the field shape to apply the appropriate
 * configuration. For "Rectangular" and "Elipsoidal" fields, it updates the field dimensions.
 * For "RTPlan" and "CustomPlan" fields, it clears previous leaf configurations, computes new
 * leaf positions for both jaws, constructs the corresponding MLC corners, and transforms these
 * to the isocentre plane if a DICOM RTPlan input is detected.
 *
 * @param control_point The control point containing configuration parameters for the MLC.
 */
void MlcSimplified::SetRunConfiguration(const ControlPoint* control_point){
    // LOGSVC_INFO("Initializing MLC Simplified for position {} and control point {}", m_isocentre, control_point->Id());
    // Update the control point
    m_control_point_id = control_point->Id();
    m_fieldShape = control_point->GetFieldType();

    auto getTransformToIsocentrePlane = [=](std::pair<G4double,G4double> pair,G4double zPosition) {
        auto ssd = 1000.0; // TODO Get from config
        return std::make_pair( ((abs(zPosition)/ ssd ) * pair.first), ((abs(zPosition)/ ssd) * pair.second) );
    };

    if(m_fieldShape == "Rectangular" || m_fieldShape == "Elipsoidal"){
        m_fieldParamA = control_point->GetFieldSizeA();
        m_fieldParamB = control_point->GetFieldSizeB();
    } else if (m_fieldShape == "RTPlan" || m_fieldShape == "CustomPlan") {
        // LOGSVC_INFO("Initializing MLC from RTPlan");
        m_mlc_a_corners.clear();
        m_mlc_b_corners.clear();
        m_mlc_corners.clear();
        const auto& mlc_a_positioning = control_point->GetMlcPositioning("Y1");
        double x_half_width = 2.5/2; // mm
        double x_init = 30 * x_half_width * 2 - x_half_width; // mm
        VMlc::m_leaves_x_positioning.clear();
        for(int leaf_idx = 0; leaf_idx < mlc_a_positioning.size(); leaf_idx++){
            double leaf_a_pos_y = mlc_a_positioning.at(leaf_idx);
            double leaf_a_pos_x = - x_init + leaf_idx * 2.5; // TEMP! fixed to 2.5 mm, TODO: getLeafAPosition(leaf_idx);
            m_mlc_a_corners.emplace_back(leaf_a_pos_y, leaf_a_pos_x - x_half_width);
            m_mlc_a_corners.emplace_back(leaf_a_pos_y, leaf_a_pos_x + x_half_width);

            VMlc::m_leaves_x_positioning.push_back(leaf_a_pos_x);
        }
        const auto& mlc_b_positioning = control_point->GetMlcPositioning("Y2");
        for(int leaf_idx = 0; leaf_idx < mlc_b_positioning.size(); leaf_idx++){
            double leaf_b_pos_y = mlc_b_positioning.at(leaf_idx);
            double leaf_b_pos_x = - x_init + leaf_idx * 2.5; // TEMP! fixed to 2.5 mm, TODO: getLeafBPosition(leaf_idx);
            m_mlc_b_corners.emplace_back(leaf_b_pos_y, leaf_b_pos_x - x_half_width);
            m_mlc_b_corners.emplace_back(leaf_b_pos_y, leaf_b_pos_x + x_half_width);
        }
        std::reverse(m_mlc_b_corners.begin(), m_mlc_b_corners.end());
        m_mlc_corners.insert(m_mlc_corners.end(), m_mlc_a_corners.begin(), m_mlc_a_corners.end());
        m_mlc_corners.insert(m_mlc_corners.end(), m_mlc_b_corners.begin(), m_mlc_b_corners.end());

        // Transform to isocentre plane once the input type is the DICOM RT_Plan
        if( dynamic_cast<IDicomPlan*>(Service<DicomSvc>()->GetPlan()) ) {
            for (int i = 0; i < m_mlc_corners.size(); i++) {
                m_mlc_corners.at(i) = getTransformToIsocentrePlane(m_mlc_corners.at(i),m_isocentre.getZ());
            }
        }
    }
    m_control_point_id = control_point->Id();
    m_isInitialized = true;
}

/**
 * @brief Checks if a given position lies within the configured MLC field.
 *
 * Determines whether the provided 3D position is inside the multi-leaf collimator (MLC) field based on its current
 * configuration (Rectangular, Elipsoidal, RTPlan, or CustomPlan). If the position's z-coordinate does not match the isocentre
 * and no transformation is requested, the method returns false. When transformation is enabled, the position is converted
 * to the mask plane before performing the appropriate in-field check:
 * - For a Rectangular field, it verifies that the absolute x and y coordinates are within preset bounds.
 * - For an Elipsoidal field, it applies the ellipse equation.
 * - For RTPlan or CustomPlan fields, it performs a point-in-polygon test.
 *
 * A critical exception is thrown if the MLC has not been initialized for the current control point.
 *
 * @param position The 3D position to evaluate.
 * @param transformToIsocentre If true, transforms the position to the isocentre (mask) plane before evaluation.
 * @return true if the position is within the MLC field; false otherwise.
 */
bool MlcSimplified::IsInField(const G4ThreeVector& position, bool transformToIsocentre) {
    auto maskLevelPosition = position;
    if((maskLevelPosition.z()!=m_isocentre.z()) && !transformToIsocentre){
        // LOGSVC_WARN("Position z {} not equal to isocentre z {}",maskLevelPosition.z(), m_isocentre.z());
        return false;
    } else if( transformToIsocentre ) {
        // G4cout << "Transforming position to isocentre plane, before: " << maskLevelPosition;
        maskLevelPosition = GetPositionInMaskPlane(position);
        // G4cout << " after: " << maskLevelPosition;
    }

    auto isPointInPolygon = [&](double x, double y){
        bool inside = false;
        // std::cout << "The m_mlc_corners.size(): " << m_mlc_corners.size() << std::endl;
        for (int i = 0, j = m_mlc_corners.size() - 1; i < m_mlc_corners.size(); j = i++) {
            if ((m_mlc_corners[i].second > y) != (m_mlc_corners[j].second > y) &&
                (x < (m_mlc_corners[j].first - m_mlc_corners[i].first) * (y - m_mlc_corners[i].second) / (m_mlc_corners[j].second - m_mlc_corners[i].second) + m_mlc_corners[i].first)) {
                inside = !inside;
                    }
                }
            // std::cout << "Inside: " << inside << " x: " << x << " y: " << y << std::endl;
            return inside;
    };

    auto current_cp= Service<RunSvc>()->CurrentControlPoint();
    if(! Initialized(current_cp)){
        G4String msg = "IsInField is called before MLC initialization for current control point.";
        msg+="\nThis #CP "+ std::to_string(m_control_point_id) + " #CurrentControlPoint: " + std::to_string(current_cp->Id());
        // LOGSVC_CRITICAL(msg.data());
        G4Exception("MlcSimplified", "IsInField", FatalErrorInArgument , msg);
    }

    if (m_fieldShape == "Rectangular"){
        if (abs(maskLevelPosition.x())<=m_fieldParamA && abs(maskLevelPosition.y())<= m_fieldParamB)
        return true;
    }
    if (m_fieldShape == "Elipsoidal"){
        if ((pow(maskLevelPosition.x(),2)/ pow(m_fieldParamA,2) + pow(maskLevelPosition.y(),2)/pow(m_fieldParamB,2))<= 1)
        return true;        
    }
    if (m_fieldShape == "RTPlan" || m_fieldShape == "CustomPlan"){
        // std::cout << "Field shape: " << m_fieldShape << std::endl;
        auto isInside = isPointInPolygon(maskLevelPosition.x(), maskLevelPosition.y());
        // if(transformToIsocentre)
        //    G4cout << " isInside: " << isInside << G4endl; 
        return isInside;
    }
    return false;
}


bool MlcSimplified::IsInField(G4PrimaryVertex* vrtx) {
    return IsInField(VMlc::GetPositionInMaskPlane(vrtx)); 
}
