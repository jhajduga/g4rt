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
 * @brief Deleted default constructor to require construction with a name.
 *
 * The class cannot be default-constructed; callers must use the explicit
 * VMlc(const std::string& name) constructor.
 */
VMlc() = delete;
        explicit VMlc(const std::string& name);
        /**
 * @brief Virtual destructor to allow proper cleanup of derived VMlc instances.
 *
 * Ensures that when a VMlc pointer to a derived object is deleted, the derived
 * class destructor is invoked so resources owned by the derived class are released.
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