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
 * @brief Restore the cell's runtime state to its initial condition.
 *
 * @note Currently a no-op: writes "Implement me." to G4cout and does not modify
 * internal state. Intended to reset voxel indices, scoring accumulators, and
 * other transient runtime state when implemented.
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
 * @brief Gets the voxel index of this cell along the Y axis.
 *
 * The returned value is the integer index of the voxel in the Y direction for this cell.
 * A value of -1 indicates the index has not been set.
 *
 * @return G4int The voxel index along Y; -1 if unset.
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
 * @brief Cell centre in the local (mother-volume) coordinate frame.
 *
 * @return G4ThreeVector Local centre coordinates (copy).
 */
    G4ThreeVector GetCentre() const { return m_centre; }

    /**
 * @brief Provides the cell center in global (world) coordinates.
 *
 * @return The cell center position in the global coordinate frame.
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
 * @brief Number of voxels along the cell's local X axis.
 *
 * @return int The number of voxels configured along the cell's local X axis.
 */
    int GetNXVoxels() const { return m_cell_voxelization_x; }
    /**
 * @brief Retrieve the number of voxels along the local Y axis.
 *
 * @return int The number of voxels along the local Y axis.
 */
int GetNYVoxels() const { return m_cell_voxelization_y; }
    /**
 * @brief Gets the number of voxels along the local Z axis of the cell.
 *
 * @return int Number of voxels along the local Z axis.
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