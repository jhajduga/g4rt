#ifndef TB_BWorldConstruction_HH
#define TB_BWorldConstruction_HH

#include "WorldConstruction.hh"
#include "D3DTray.hh"



class BWorldConstruction: public WorldConstruction {
public:
  static WorldConstruction *GetInstance();

bool Create() override;

void ConstructTrayDetectors(G4VPhysicalVolume *parentPV);

void ConstructSDandField() override;

std::vector<VPatient*> GetCustomDetectors() const override;

private:
    ///
    BWorldConstruction();

    ///
    ~BWorldConstruction();

    /// Delete the copy and move constructors
    BWorldConstruction(const BWorldConstruction &) = delete;
    BWorldConstruction &operator=(const BWorldConstruction &) = delete;
    BWorldConstruction(BWorldConstruction &&) = delete;
    BWorldConstruction &operator=(BWorldConstruction &&) = delete;

    ///
    std::vector<D3DTray*> m_trays;

    ///
    G4VPhysicalVolume* m_bunker_wall = nullptr;
    G4VPhysicalVolume* m_bunker_inside_pv = nullptr;

};

#endif