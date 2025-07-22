#ifndef VMLC_HH
#define VMLC_HH

#include "G4VPhysicalVolume.hh"
#include "Types.hh"
class ControlPoint;
class G4PrimaryVertex;

class VMlc: public RunComponet {
    private:
        ///
        void AcceptRunVisitor(RunSvc *visitor) override;

    protected:
        std::vector<G4VPhysicalVolumeUPtr> m_y1_leaves;
        std::vector<G4VPhysicalVolumeUPtr> m_y2_leaves; 

        // Vector of leaf positions in X direction, to be initialized
        // in derived classes!!!
        std::vector<G4double> m_leaves_x_positioning;

        int m_control_point_id = -1;
        bool m_isInitialized = false;
        static G4ThreeVector m_isocentre;
    public:
        /**
 * @brief Deleted default constructor to enforce construction with a name.
 *
 * Prevents instantiation of VMlc without specifying a name parameter.
 */
VMlc() = delete;
        explicit VMlc(const std::string& name);
        /**
 * @brief Virtual destructor for the VMlc class.
 *
 * Ensures proper cleanup of resources in derived multi-leaf collimator classes.
 */
virtual ~VMlc() = default;
        virtual bool IsInField(const G4ThreeVector& position, bool transformToIsocentre=false) = 0;
        virtual bool IsInField(G4PrimaryVertex* vrtx) = 0;
        std::vector<G4ThreeVector> GetMlcPositioning(const std::string& side) const;
        G4ThreeVector GetMaskCentre() const;
        bool Initialized(const ControlPoint* control_point) const;
        static G4ThreeVector GetPositionInMaskPlane(const G4ThreeVector& position);
        static G4ThreeVector GetPositionInMaskPlane(const G4PrimaryVertex* vrtx);

        static G4double GetMlcZPosition();
};
#endif // VMLC_HH