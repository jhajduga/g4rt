#ifndef TB_TLDWorldConstruction_HH
#define TB_TLDWorldConstruction_HH

#include "WorldConstruction.hh"
#include "TLDTray.hh"



class TLDWorldConstruction: public WorldConstruction {
public:
  static WorldConstruction *GetInstance();

bool Create() override;

void ConstructTLDTray(G4VPhysicalVolume *parentPV);

void ConstructSDandField() override;

private:
    /**
 * @brief Default constructor for TLDWorldConstruction.
 *
 * Initializes a new instance of the TLDWorldConstruction class. This constructor is private to enforce the singleton pattern.
 */
    TLDWorldConstruction() = default;
    /**
 * @brief Destroys the TLDWorldConstruction instance.
 *
 * Cleans up resources associated with the TLD world construction when the singleton instance is destroyed.
 */
~TLDWorldConstruction() = default;

    /**
 * @brief Deleted copy constructor to prevent copying of the singleton instance.
 */
    TLDWorldConstruction(const TLDWorldConstruction &) = delete;
    /**
 * @brief Deleted copy assignment operator to prevent copying of TLDWorldConstruction instances.
 */
TLDWorldConstruction &operator=(const TLDWorldConstruction &) = delete;
    /**
 * @brief Move constructor is deleted to prevent moving of TLDWorldConstruction instances.
 */
TLDWorldConstruction(TLDWorldConstruction &&) = delete;
    /**
 * @brief Deleted move assignment operator to prevent moving of TLDWorldConstruction instances.
 */
TLDWorldConstruction &operator=(TLDWorldConstruction &&) = delete;

    ///
    TLDTray* m_tld_tray;
};

#endif