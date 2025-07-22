
#ifndef DOSE3DMLCSIMPLIFIED_HH
#define DOSE3DMLCSIMPLIFIED_HH

#include "VMlc.hh"

class MlcSimplified : public VMlc {
    private:
        std::string m_fieldShape;
        G4double m_fieldParamA = 0.0;
        G4double m_fieldParamB = 0.0;
        std::vector<std::pair<double,double>> m_mlc_a_corners;
        std::vector<std::pair<double,double>> m_mlc_b_corners;
        std::vector<std::pair<double,double>> m_mlc_corners;

public:
    MlcSimplified();
    ~MlcSimplified() override {};	
    bool IsInField(const G4ThreeVector& position, bool transformToIsocentre = false) override;
    bool IsInField(G4PrimaryVertex* vrtx) override;
    void SetRunConfiguration(const ControlPoint* ) override;


};

#endif //DOSE3DMLCSIMPLIFIED_HH
