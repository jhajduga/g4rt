/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 29.03.2023
*
*/

#ifndef Dose3D_ControlPoint_H
#define Dose3D_ControlPoint_H

#include "Types.hh"
#include "VoxelHit.hh"
#include "TFile.h"
#include "G4Cache.hh"
#include "VPatient.hh"
#include "G4Run.hh"
#include "VoxelHit.hh"
#include "G4Threading.hh" 

typedef std::map<Scoring::Type, std::map<std::size_t, VoxelHit>> ScoringMap;

class ControlPoint;
class VMlc;

class ControlPointConfig {
  public:
    /**
 * @brief Constructs a ControlPointConfig with default parameters.
 */
ControlPointConfig() = default;
    ControlPointConfig(int id, int nevts, double rot);
    double RotationInDeg = 0.;
    int NEvts = 0.;
    int Id = 0;
    std::string PlanFile = std::string();
    // TODO: introduce here FieldType as enum type see def in Types.hh
    std::string FieldType = std::string();
    G4double FieldSizeA = G4double();
    G4double FieldSizeB = G4double();
};

class ControlPointRun : public G4Run {
  private:
    /// RunCollectionName / ScoringMap
    mutable std::map<G4String,ScoringMap> m_hashed_scoring_map;

    mutable std::vector<G4ThreeVector> m_sim_mask_points;
    
    ///
    void InitializeScoringCollection();

    ///
    void FillMlcFieldScalingFactor();

    ///
    void FillParameterization();

    ///
    double m_beam_mask_area = 1;
    std::pair<double, double> m_beam_mask_gravity_centre = {1,1};
    

  public:
    /**
     * @brief Construct a ControlPointRun.
     *
     * Creates a per-control-point G4Run. If `scoring` is true, scoring collections
     * required for this run are initialized.
     *
     * @param scoring Whether to initialize scoring collections for the run (default: false).
     */
    ControlPointRun(bool scoring=false) {
      if(scoring)
        InitializeScoringCollection();
    };

    /**
     * @brief Releases resources held by the ControlPointRun.
     *
     * Logs a debug message to G4cout when destroyed.
     */
    ~ControlPointRun(){
      G4cout << "DESTRUCOTOR OF ControlPointRun..." << G4endl;
    };

    ///
    void Merge(const G4Run* aRun) override;

    ///
    ScoringMap& GetScoringCollection(const G4String& name);

    /**
 * @brief Access the scoring collections recorded for this run.
 *
 * Provides a const reference to the map that associates run collection names with their scoring data.
 *
 * @return Const reference to the map from run collection name to its ScoringMap.
 */
    const std::map<G4String,ScoringMap>& GetScoringCollections() const {return m_hashed_scoring_map;}

    /**
 * @brief Access the mutable vector of simulation mask points for this run.
 *
 * Allows reading and modifying the run's simulation mask points; modifications apply to this ControlPointRun instance.
 *
 * @return std::vector<G4ThreeVector>& Reference to the simulation mask points vector.
 */
    std::vector<G4ThreeVector>& GetSimMaskPoints() {return m_sim_mask_points;}
    /**
 * @brief Accesses the simulation mask points for this run.
 *
 * @return const reference to the vector of 3D points that define the simulation mask.
 */
const std::vector<G4ThreeVector>& GetSimMaskPoints() const {return m_sim_mask_points;}

    ///
    void EndOfRun();

    /**
 * @brief Beam mask area for this run.
 *
 * @return double Beam mask area.
 */
    double GetBeamMaskArea() const { return m_beam_mask_area; }

    /**
 * @brief Return the beam mask gravity center coordinates.
 *
 * Returns the (x, y) coordinates of the beam mask gravity center in the simulation's length units.
 *
 * @return std::pair<double,double> Pair {x, y} representing the gravity center.
 */
    std::pair<double, double> GetBeamMaskeGravCentre() const { return m_beam_mask_gravity_centre; }
};

class ControlPoint {
  public:
    ControlPoint(const ControlPointConfig& config);
    ControlPoint(const ControlPoint& cp);
    ControlPoint(ControlPoint&& cp);
    ~ControlPoint();
    /**
 * @brief Get the control point identifier.
 *
 * @return int The control point identifier.
 */
int GetId() const { return m_config.Id; }
    /**
 * @brief Get the treatment plan filename associated with this control point.
 *
 * @return The treatment plan filename.
 */
std::string GetPlanFile() const { return m_config.PlanFile; }
    /**
 * @brief Number of events configured for this control point.
 *
 * @return int Number of events configured for the control point.
 */
int GetNEvts() const { return m_config.NEvts; }
    /**
 * @brief Access the control point's rotation matrix.
 *
 * Returns a non-owning pointer to the internal G4RotationMatrix describing this control point's orientation.
 * The pointer may be `nullptr` if no rotation has been set; the caller must not delete or take ownership of it.
 *
 * @return G4RotationMatrix* Pointer to the rotation matrix, or `nullptr` if unset.
 */
G4RotationMatrix* GetRotation() const { return m_rotation; }
    /**
 * @brief Rotation angle of the control point in degrees.
 *
 * @return G4double Rotation angle in degrees.
 */
G4double GetDegreeRotation() const {return m_config.RotationInDeg;}
    void SetRotation(double rotationInDegree);
    /**
 * @brief Sets the number of simulation events for the control point.
 *
 * @param nevts The new number of events to assign.
 */
void SetNEvts(int nevts) { m_config.NEvts = nevts; }
    G4double GetFieldScalingFactor(const G4ThreeVector& position) const;
    G4double GetAngleScalingFactor(G4double angle, const G4ThreeVector& position) const;

    const std::vector<G4ThreeVector>& GetFieldMask(const std::string& type="Plan");
    
    void DumpVolumeMaskToFile(std::string scoring_vol_name, const std::map<std::size_t, VoxelHit>& volume_scoring) const;
    std::string GetSimOutputTFileName(bool workerMT = false) const;

    static std::string GetOutputDir();

    G4Run* GenerateRun(bool scoring=false);

    /**
 * @brief Return the thread-local run instance for this control point.
 *
 * The returned pointer is non-owning and refers to the ControlPointRun for the calling thread.
 * It may be `nullptr` if no run has been created for this control point on the current thread.
 *
 * @return ControlPointRun* Pointer to the current thread-local ControlPointRun, or `nullptr` if unset.
 */
ControlPointRun* GetRun() {return m_cp_run.Get();}
    /**
 * @brief Get the thread-local current run associated with this control point.
 *
 * @return `const ControlPointRun*` pointing to the current run, or `nullptr` if no run is set.
 */
const ControlPointRun* GetRun() const {return m_cp_run.Get();}

    /**
 * @brief Configured radiation field type for the control point.
 *
 * @return std::string The field type as specified in the control point configuration.
 */
std::string GetFieldType() const { return m_config.FieldType; } 

    /**
 * @brief Field size along the A axis for this control point.
 *
 * @return Field size along the A axis in the configured units.
 */
G4double GetFieldSizeA() const { return m_config.FieldSizeA; }

    /**
 * @brief Get the radiation field dimension along the B axis for this control point.
 *
 * Returns the configured field size along the B axis (same units as stored in the control-point configuration).
 *
 * @return G4double Field size along the B axis.
 */
G4double GetFieldSizeB() const { return m_config.FieldSizeB; }
    
    /**
 * @brief Gets the control point identifier.
 *
 * @return int Control point identifier.
 */
int Id() const { return m_config.Id; }

    std::string GetOutputFileName() const;

    /**
 * @brief Get the data type names associated with this control point.
 *
 * @return Reference to a vector of strings containing the data type names (for example, "Plan" and "Sim").
 */
const std::vector<std::string>& DataTypes() const { return m_data_types; }

    void FillEventCollections(G4HCofThisEvent* evtHC);

    void FillSimFieldMask(const std::vector<G4PrimaryVertex*>& p_vrtx);

    ///
    static std::vector<G4String> GetRunCollectionNames();
    static std::set<G4String> GetHitCollectionNames();

    const std::vector<double>& GetMlcPositioning(const std::string& side) const;
    double GetJawAperture(const std::string& side) const;

    /**
 * @brief Get the stored plan field mask points for this control point.
 *
 * The returned vector contains the 3D points that define the plan mask. Modifying
 * the vector directly updates the ControlPoint's stored mask points.
 *
 * @return std::vector<G4ThreeVector>& Reference to the internal plan mask points.
 */
std::vector<G4ThreeVector>& GetPlanMaskPoints() {return m_plan_mask_points;}
    void FillPlanFieldMask();
    VMlc* MLC() const;
  private:
    friend class ControlPointRun;
    friend class VPatientSD;

    ControlPointConfig m_config;
    static std::string m_sim_dir;
    std::vector<std::string> m_data_types={"Plan", "Sim"};

    G4RotationMatrix* m_rotation = nullptr;

    /// Jaw Aperture in mm
    std::pair<double,double> m_jaw_x_aperture;
    std::pair<double,double> m_jaw_y_aperture;

    /// MLC positioning in mm
    std::vector<double> m_mlc_a_positioning;
    std::vector<double> m_mlc_b_positioning;

    std::set<Scoring::Type> m_scoring_types;

    std::vector<G4ThreeVector> m_plan_mask_points;

    /// Store to kepp raw pointers from ControlPoint::GenerateRun
    G4Cache<std::vector<ControlPointRun*>> m_mt_run;

    /// MTRunManager generates new run on each thread
    G4Cache<ControlPointRun*> m_cp_run;

    /// Many HitsCollections can be associated to given run collection name 
    // (e.g. when many sensitive detectors constituting a single detection unit a.k.a ROI)
    static G4Cache<std::map<G4String,std::vector<G4String>>> m_run_collections;
    static void RegisterRunHCollection(const G4String& collection_name, const G4String& hc_name);

    static double FIELD_MASK_POINTS_DISTANCE;
    void FillPlanFieldMaskForRegularShapes(double current_z);
    void FillPlanFieldMaskForInputPlan(double current_z);
    void FillEventCollection(const G4String& run_collection, VoxelHitsCollection* hitsColl);

};

#endif //Dose3D_ControlPoint_H