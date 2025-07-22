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
    ///
    TLDWorldConstruction() = default;
    ~TLDWorldConstruction() = default;

    /// Delete the copy and move constructors
    TLDWorldConstruction(const TLDWorldConstruction &) = delete;
    TLDWorldConstruction &operator=(const TLDWorldConstruction &) = delete;
    TLDWorldConstruction(TLDWorldConstruction &&) = delete;
    TLDWorldConstruction &operator=(TLDWorldConstruction &&) = delete;

    ///
    TLDTray* m_tld_tray;
};

#endif