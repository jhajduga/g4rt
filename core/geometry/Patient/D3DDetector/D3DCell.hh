/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 06.05.2022
*
*/

#ifndef D3D_CELL_HH
#define D3D_CELL_HH

#include "G4PVPlacement.hh"
#include "Configurable.hh"
#include "VPatient.hh"

///\class D3DCell
///\brief The Phantom filled with scintilator
class D3DCell : public VPatient {
  public:
    ///
    D3DCell(const G4String& label = "Dose3DCell", const G4ThreeVector& centre = G4ThreeVector(), G4String cellMediumName = "PMMA");

    ///
    ~D3DCell();

    ///
    void Construct(G4VPhysicalVolume *parentPV) override;

    ///
    void SetNVoxels(char axis, int nv);

    ///
    void Destroy() override;

    ///
    G4bool Update() override;

    ///
    void Reset() override { G4cout << "Implement me." << G4endl; }

    ///
    void WriteInfo() override;

    ///
    void DefineSensitiveDetector() override;

    ///
    void SetIDs(G4int x, G4int y, G4int z);
    
    ///
    G4int GetIdX() const { return m_id_x; }
    G4int GetIdY() const { return m_id_y; }
    G4int GetIdZ() const { return m_id_z; }

    ///
    G4ThreeVector GetCentre() const { return m_centre; }

    ///
    G4ThreeVector GetGlobalCentre() const { return m_global_centre; }


    ///
    static G4ThreeVector SIZE;

    ///
    void ParseTomlConfig() override {}

    ///
    bool IsRunCollectionScoringVolumeVoxelised(const G4String& run_collection) const;

    ///
    int GetNXVoxels() const { return m_cell_voxelization_x; }
    int GetNYVoxels() const { return m_cell_voxelization_y; }
    int GetNZVoxels() const { return m_cell_voxelization_z; }

    ///
    void static CellScorer(G4bool val);

    ///
    void static CellVoxelisedScorer(G4bool val);


  private:
    ///
    void SetGlobalCentre(const G4ThreeVector& centre) {m_global_centre = centre; }

    /// pointer to user step limits
    G4UserLimits* m_step_limit = nullptr;

    /// 
    G4int m_cell_voxelization_x = 1;
    G4int m_cell_voxelization_y = 1;
    G4int m_cell_voxelization_z = 1;

    G4String m_cell_label;

    ///
    G4String m_cell_medium;

    /// Local position, i.e. the position in the mother volume frame
    G4ThreeVector m_centre;

    /// Once the cells positioning is being defined within the patiend environemt 
    /// volume global psoition will be different than local one
    G4ThreeVector m_global_centre;

    ///
    G4int m_id_x = -1;
    G4int m_id_y = -1;
    G4int m_id_z = -1;

    ///
    static G4bool m_set_cell_scorer;

    ///
    static G4bool m_set_cell_voxelised_scorer;
};

#endif  // D3D_CELL_HH
