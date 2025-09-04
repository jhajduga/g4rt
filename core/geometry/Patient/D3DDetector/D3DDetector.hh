/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 16.05.2022
*
*/

#ifndef D3D_MODULE_HH
#define D3D_MODULE_HH

#include "G4PVPlacement.hh"
#include "D3DCell.hh"
#include "Types.hh"
#include <string>

///\class D3DDetector
///\brief The Phantom constructed on top of Dose3D cells
class D3DDetector : public VPatient, public GeoComponet {
  public:


    ///
    D3DDetector(const std::string& label = "D3DDetector");

    ///
    ~D3DDetector();

    ///
    void Construct(G4VPhysicalVolume *parentPV) override;

    ///
    void Destroy() override;

    ///
    G4bool Update() override;

    /**
 * @brief Reset detector state to its initial condition.
 *
 * This is a placeholder implementation and does not modify detector state.
 * Currently it emits a message to G4cout indicating it is unimplemented.
 *
 * Implementations should restore any mutable runtime state (counters, cached
 * data, per-run accumulators, etc.) to their defaults.
 */
    void Reset() override { G4cout << "Implement me." << G4endl; }

    ///
    void WriteInfo() override;

    ///
    void DefineSensitiveDetector() override;

    /// 
    std::string SetGeometrySource();
    
    ///
    bool IsAnyCellVoxelised(const G4String& run_collection) const;

    ///
    void ExportCellPositioningToCsv(const std::string& path_to_output_dir) const override;
    void ExportVoxelPositioningToCsv(const std::string& path_to_output_dir) const override;
    void ExportPositioningToTFile(const std::string& path_to_output_dir) const override;
    void ExportToGateCsv(const std::string& path_to_output_dir) const override;

    //
    std::map<std::size_t, VoxelHit> GetScoringHashedMap(const G4String& scoring_name,Scoring::Type type) const override;

    /**
 * @brief Returns the volume of a single Dose3D cell.
 *
 * The volume is calculated as the product of the cell's dimensions along the X, Y, and Z axes.
 *
 * @return G4double The computed cell volume.
 */
    G4double GetCellVolume() const override { return D3DCell::SIZE.getX() * D3DCell::SIZE.getY() * D3DCell::SIZE.getZ(); };

    class Config {
      public:
        std::string m_cell_medium = "None";

        // Content of geometry prodyction from tsl file format
        G4String m_stl_geometry_file_path = "None";
        G4String m_stl_positioning_file_path = "None";


        G4int m_nX_cells = 0;
        G4int m_nY_cells = 0;
        G4int m_nZ_cells = 0;

        G4ThreeVector m_cell_position = {0., 0., 0.};


        /// 
        G4ThreeVector m_translation_in_local_frame;

        ///
        G4int m_cell_nX_voxels = 0;
        G4int m_cell_nY_voxels = 0;
        G4int m_cell_nZ_voxels = 0;

        bool m_initialized = false;
    };

    ///
    void SetConfig(const D3DDetector::Config& config);

    /**
 * @brief Return a read-only reference to the detector configuration.
 *
 * Provides const access to the internal D3DDetector::Config used by this instance.
 *
 * @return const D3DDetector::Config& Reference to the internal configuration object.
 */
    const D3DDetector::Config& GetConfig() const { return m_config; }

    /**
 * @brief Indicates whether a point is inside the detector volume.
 *
 * Always returns false, indicating that no point is considered inside the detector.
 *
 * @param x X-coordinate of the point.
 * @param y Y-coordinate of the point.
 * @param z Z-coordinate of the point.
 * @return G4bool Always false.
 */
    G4bool IsInside(double x, double y, double z) override { return false; }
    ///
    static G4double COVER_WIDTH;

    ///
  private:

    ///
    G4bool LoadParameterization();

    ///
    G4bool LoadDefaultParameterization();

    ///
    void ParseTomlConfig() override;

    ///
    void AcceptGeoVisitor(GeoSvc *visitor) const override;

    ///
    G4String m_label;

    ///
    D3DDetector::Config m_config;

    ///
    std::vector<D3DCell*> m_d3d_cells;
    
    /// Container for cells positioning read-in from file
    std::vector<G4ThreeVector> m_d3d_cells_positioning;

    ///
    void ReadCellsPositioning();
    void ComputeRegularCellPositioning();

    ///
    static std::map<std::string, std::map<std::size_t, VoxelHit>> m_hashed_scoring_map_template; 
};

#endif  // D3D_MODULE_HH
