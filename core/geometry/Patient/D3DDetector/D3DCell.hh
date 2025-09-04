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
 * @brief Reset the cell to its initial state.
 *
 * Currently a no-op: it writes "Implement me." to G4cout and does not modify
 * internal state. Intended to be implemented to restore voxel indices, scoring
 * accumulators, and any transient runtime state.
 */
    void Reset() override { G4cout << "Implement me." << G4endl; }

    ///
    void WriteInfo() override;

    ///
    void DefineSensitiveDetector() override;

    ///
    void SetIDs(G4int x, G4int y, G4int z);
    
    /**
 * @brief Get the voxel index along the local x-axis.
 *
 * Returns the integer index of this cell in the X voxel grid.
 * A value of -1 indicates the index has not been set.
 *
 * @return G4int Voxel index along X (or -1 if unset).
 */
    G4int GetIdX() const { return m_id_x; }
    /**
 * @brief Return the voxel index of this cell along the Y axis.
 *
 * Returns the integer index of the voxel in the Y direction for this cell.
 * A value of -1 indicates the index has not been set.
 *
 * @return G4int Voxel index along Y (or -1 if unset).
 */
G4int GetIdY() const { return m_id_y; }
    /**
 * @brief Get the voxel index of this cell along the z axis.
 *
 * Returns the integer voxel index assigned to this cell for the z coordinate.
 * A negative value (default -1) indicates the index has not been set.
 *
 * @return G4int Voxel index along z (or negative if unset).
 */
G4int GetIdZ() const { return m_id_z; }

    /**
 * @brief Get the cell center in the local (mother-volume) coordinate frame.
 *
 * @return G4ThreeVector Local center coordinates of the cell (copy).
 */
    G4ThreeVector GetCentre() const { return m_centre; }

    /**
 * @brief Get the cell center position expressed in global (world) coordinates.
 *
 * @return G4ThreeVector The cell center position in the global coordinate frame.
 */
    G4ThreeVector GetGlobalCentre() const { return m_global_centre; }


    ///
    static G4ThreeVector SIZE;

    /**
 * @brief No-op TOML configuration hook required by the interface.
 *
 * Exists to satisfy the VPatient interface; this implementation does not
 * parse or apply any TOML configuration.
 */
    void ParseTomlConfig() override {}

    ///
    bool IsRunCollectionScoringVolumeVoxelised(const G4String& run_collection) const;

    /**
 * @brief Number of voxels configured along the cell's local X axis.
 *
 * @return int The configured voxel count along X.
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
