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

    /**
 * @brief Deleted copy constructor to prevent copying of the singleton instance.
 */
    BWorldConstruction(const BWorldConstruction &) = delete;
    /**
 * @brief Deleted copy assignment operator to prevent copying of BWorldConstruction instances.
 */
BWorldConstruction &operator=(const BWorldConstruction &) = delete;
    /**
 * @brief Deleted move constructor — instances of BWorldConstruction are not movable.
 *
 * This prevents transfer of internal ownership/state by move operations.
 */
BWorldConstruction(BWorldConstruction &&) = delete;
    /**
 * @brief Deleted move assignment operator to prevent moving of BWorldConstruction instances.
 */
BWorldConstruction &operator=(BWorldConstruction &&) = delete;

    ///
    std::vector<D3DTray*> m_trays;

    ///
    G4VPhysicalVolume* m_bunker_wall = nullptr;
    G4VPhysicalVolume* m_bunker_inside_pv = nullptr;

};

#endif