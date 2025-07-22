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

    /**
 * @brief Placeholder method for resetting the cell state.
 *
 * Currently not implemented; outputs a message indicating the need for implementation.
 */
    void Reset() override { G4cout << "Implement me." << G4endl; }

    ///
    void WriteInfo() override;

    ///
    void DefineSensitiveDetector() override;

    ///
    void SetIDs(G4int x, G4int y, G4int z);
    
    /**
 * @brief Returns the voxel index of the cell along the x-axis.
 *
 * @return G4int Voxel index in the x direction.
 */
    G4int GetIdX() const { return m_id_x; }
    /**
 * @brief Returns the voxel index of the cell along the y-axis.
 *
 * @return G4int Voxel index in the y direction.
 */
G4int GetIdY() const { return m_id_y; }
    /**
 * @brief Returns the voxel index of the cell along the z-axis.
 *
 * @return G4int Voxel index in the z direction.
 */
G4int GetIdZ() const { return m_id_z; }

    /**
 * @brief Returns the local center position of the cell.
 *
 * @return G4ThreeVector The local center coordinates of the cell.
 */
    G4ThreeVector GetCentre() const { return m_centre; }

    /**
 * @brief Returns the global center position of the cell.
 *
 * @return G4ThreeVector Global coordinates of the cell center.
 */
    G4ThreeVector GetGlobalCentre() const { return m_global_centre; }


    ///
    static G4ThreeVector SIZE;

    /**
 * @brief Placeholder for TOML configuration parsing.
 *
 * This method is intentionally left empty and does not perform any configuration parsing.
 */
    void ParseTomlConfig() override {}

    ///
    bool IsRunCollectionScoringVolumeVoxelised(const G4String& run_collection) const;

    /**
 * @brief Returns the number of voxels along the x-axis for this cell.
 *
 * @return int Number of voxels in the x direction.
 */
    int GetNXVoxels() const { return m_cell_voxelization_x; }
    /**
 * @brief Returns the number of voxels along the y-axis in the cell.
 *
 * @return int Number of voxels along the y-axis.
 */
int GetNYVoxels() const { return m_cell_voxelization_y; }
    /**
 * @brief Returns the number of voxels along the z-axis in the cell.
 *
 * @return int Number of z-axis voxels.
 */
int GetNZVoxels() const { return m_cell_voxelization_z; }

    ///
    void static CellScorer(G4bool val);

    ///
    void static CellVoxelisedScorer(G4bool val);


  private:
    /**
 * @brief Sets the global center position of the cell.
 *
 * @param centre The new global center position as a G4ThreeVector.
 */
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
