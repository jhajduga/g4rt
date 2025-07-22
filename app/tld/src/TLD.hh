/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 21.06.2024
*
*/

#ifndef TLDL_HH
#define TLDL_HH

#include "G4PVPlacement.hh"
#include "Configurable.hh"
#include "VPatient.hh"
#include "CADMesh.hh"
#undef Error
#undef Next
// FIXME: ROOT się gryzie z CADMeshem i oba mają tak samo nazwane makra zdefiniowane i problem z kolejnością includów czy coś takiego - nie do końca ogarniam.
// Wyższy poziom C++ tu już wchodzi. 

///\class TLD
class TLD : public VPatient {
  public:
    ///
    TLD(const G4String& label = "TLD", const G4ThreeVector& centre = G4ThreeVector(), G4String tldMediumName = "LiF:Mg,Ti", G4String stlGeometryFilePath = "None");

    ///
    ~TLD();

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
    static G4double SIZE;

    ///
    void ParseTomlConfig() override {}

    ///
    bool IsRunCollectionScoringVolumeVoxelised(const G4String& run_collection) const;

    ///
    int GetNXVoxels() const { return m_tld_voxelization_x; }
    int GetNYVoxels() const { return m_tld_voxelization_y; }
    int GetNZVoxels() const { return m_tld_voxelization_z; }

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
    G4int m_tld_voxelization_x = 1;
    G4int m_tld_voxelization_y = 1;
    G4int m_tld_voxelization_z = 1;

    ///
    G4String m_tld_medium;

    /// Path to the file with the geometry of the TLD
    /// if provided the geometry is being created from the STL file
    /// otherwise the geometry is being created from the G4Tube
    G4String m_stl_geometry_file_path;

    /// STL mesh pointer
    std::shared_ptr<CADMesh::TessellatedMesh> m_stlMesh;

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
    static G4bool m_set_tld_scorer;

    ///
    static G4bool m_set_tld_voxelised_scorer;

};

#endif  // TLDL_HH
