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
     * @brief Destructor.
     *
     * Releases any resources held by the ControlPointRun instance.
     * Emits a debug message to G4cout when invoked.
     */
    ~ControlPointRun(){
      G4cout << "DESTRUCOTOR OF ControlPointRun..." << G4endl;
    };

    ///
    void Merge(const G4Run* aRun) override;

    ///
    ScoringMap& GetScoringCollection(const G4String& name);

    /**
 * @brief Get all scoring collections recorded for this run.
 *
 * Returns a const reference to the internal map that associates run collection
 * names with their ScoringMap (mapping Scoring::Type to per-index VoxelHit maps).
 *
 * @return const std::map<G4String,ScoringMap>& Map from run collection name to scoring data.
 */
    const std::map<G4String,ScoringMap>& GetScoringCollections() const {return m_hashed_scoring_map;}

    /**
 * @brief Access the mutable container of simulation mask points for this run.
 *
 * Returns a reference to the internal vector of G4ThreeVector that stores
 * simulation mask points for the run. Callers may read or modify the vector;
 * changes apply to this ControlPointRun instance.
 *
 * @return std::vector<G4ThreeVector>& Reference to the simulation mask points.
 */
    std::vector<G4ThreeVector>& GetSimMaskPoints() {return m_sim_mask_points;}
    /**
 * @brief Returns the simulation mask points for the control point run.
 *
 * @return Reference to a vector of 3D points representing the simulation mask.
 */
const std::vector<G4ThreeVector>& GetSimMaskPoints() const {return m_sim_mask_points;}

    ///
    void EndOfRun();

    /**
 * @brief Returns the area of the beam mask for the control point run.
 *
 * @return double Area of the beam mask.
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
 * @brief Returns the identifier of the control point.
 *
 * @return int The control point's unique ID.
 */
int GetId() const { return m_config.Id; }
    /**
 * @brief Returns the filename of the treatment plan associated with this control point.
 *
 * @return std::string The treatment plan filename.
 */
std::string GetPlanFile() const { return m_config.PlanFile; }
    /**
 * @brief Returns the number of events configured for this control point.
 *
 * @return int Number of events.
 */
int GetNEvts() const { return m_config.NEvts; }
    /**
 * @brief Get the control point's rotation matrix.
 *
 * Returns the internal G4RotationMatrix that describes this control point's orientation.
 * May be nullptr if no rotation has been set. The caller must not delete or take ownership
 * of the returned pointer.
 *
 * @return G4RotationMatrix* Pointer to the rotation matrix, or nullptr if unset.
 */
G4RotationMatrix* GetRotation() const { return m_rotation; }
    /**
 * @brief Returns the rotation angle of the control point in degrees.
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
 * @brief Get the thread-local run instance for this control point.
 *
 * Returns the pointer to the ControlPointRun associated with the calling thread.
 * This is a non-owning pointer; it may be nullptr if no run has been created for
 * this control point on the current thread.
 *
 * @return ControlPointRun* Pointer to the current thread-local ControlPointRun, or nullptr if unset.
 */
ControlPointRun* GetRun() {return m_cp_run.Get();}
    /**
 * @brief Returns the current run instance associated with this control point.
 *
 * @return Pointer to the current `ControlPointRun` object, or nullptr if not set.
 */
const ControlPointRun* GetRun() const {return m_cp_run.Get();}

    /**
 * @brief Returns the type of radiation field for the control point.
 *
 * @return std::string The field type as specified in the control point configuration.
 */
std::string GetFieldType() const { return m_config.FieldType; } 

    /**
 * @brief Get the radiation field size along the A axis for this control point.
 *
 * @return G4double Field size along the A axis.
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
 * @brief Returns the identifier of the control point.
 *
 * @return int Control point ID.
 */
int Id() const { return m_config.Id; }

    std::string GetOutputFileName() const;

    /**
 * @brief Returns the list of data type names associated with the control point.
 *
 * @return Reference to a vector of strings representing data types (e.g., "Plan", "Sim").
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
 * @brief Access the plan field mask points for this control point.
 *
 * Returns a reference to the internal vector of 3D points that define the plan mask.
 * Modifying the returned vector alters the ControlPoint's stored mask points.
 *
 * @return std::vector<G4ThreeVector>& Reference to the plan mask points.
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