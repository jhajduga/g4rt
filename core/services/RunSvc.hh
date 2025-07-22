/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 10.01.2018
*
*/

#ifndef Dose3D_RUNSVC_H
#define Dose3D_RUNSVC_H

#include "TomlConfigurable.hh"
#include "Types.hh"
#include "G4RunManager.hh"
#include "ControlPoint.hh"
#include "VoxelHit.hh"
#include "LogSvc.hh"

#define RUNSVC_MODULE "RunSvc"

#define RUNSVC_DEBUG(msg, ...)   LOGSVC_DEBUG(RUNSVC_MODULE, msg, ##__VA_ARGS__)
#define RUNSVC_INFO(msg, ...)    LOGSVC_INFO(RUNSVC_MODULE, msg, ##__VA_ARGS__)
#define RUNSVC_WARNING(msg, ...) LOGSVC_WARN(RUNSVC_MODULE, msg, ##__VA_ARGS__)
#define RUNSVC_ERROR(msg, ...)   LOGSVC_ERROR(RUNSVC_MODULE, msg, ##__VA_ARGS__)
#define RUNSVC_FATAL(msg, ...)   LOGSVC_FATAL(RUNSVC_MODULE, msg, ##__VA_ARGS__)
/// TODO 1: TpFractionCounter (double evtTotalEnergy); // return EvtFractionId
/// TODO 2: DaqTimeCounter(); // based on the global timer, returns EvtTimeId

class G4RunManager;

////////////////////////////////////////////////////////////////////////////////
///
/// \enum OperationalMode
/// \brief Indicate different operational modes of application run
enum class OperationalMode {
  BuildGeometry,
  FullSimulation
};

////////////////////////////////////////////////////////////////////////////////
///
///\class RunSvc
///\brief The application run management service.
/// It is a singleton type the pointer of which can be asses trough the templated method:
/// Service<RunSvc>()
class RunSvc : public TomlConfigurable {
  private:



  RunSvc();

  ~RunSvc();

  /**
 * @brief Deleted copy constructor to enforce singleton pattern.
 *
 * Prevents copying of the RunSvc instance.
 */
  RunSvc(const RunSvc &) = delete;

  /**
 * @brief Deleted copy assignment operator to enforce singleton pattern.
 *
 * Prevents copying of the RunSvc instance.
 */
RunSvc &operator=(const RunSvc &) = delete;

  /**
 * @brief Move constructor is deleted to enforce singleton pattern.
 */
RunSvc(RunSvc &&) = delete;

  /**
 * @brief Move assignment operator is deleted to enforce singleton behavior.
 *
 * Prevents moving assignment of the RunSvc instance.
 */
RunSvc &operator=(RunSvc &&) = delete;

  ///\brief Keep info about the actual application mode status.
  OperationalMode m_application_mode;

  ///\brief Container of .mac files to be processed in the Geant4 framework.
  std::vector<G4String> m_macFiles;

  ///\brief Keep pointer to the Geant4 run manager.
  G4RunManager *m_g4RunManager = nullptr;

  ///\brief Keep track of service initialization status.
  bool m_isInitialized = false;

  ///\brief Keep track of the user's Geant4 initialization status.
  bool m_isUsrG4Initialized = false;

  ///
  std::set<Scoring::Type> m_scoring_types={Scoring::Type::Cell, Scoring::Type::Voxel};
  std::vector<ControlPointConfig> m_control_points_config;
  std::vector<ControlPoint> m_control_points;

  ///
  std::vector<RunComponet*> m_run_components;

  ///
  ControlPoint* m_current_control_point = nullptr;

  ///\brief Virtual method implementation defining the list of configuration units for this module.
  void Configure() override;

  ///
  void DefineSimConfiguration();
  void DefineSimDefaultConfig();

  ///\brief Perform User's configuration within the Geant4 framework.
  void UserG4Initialization();
  
  /// @brief Simpple method to check initialization of each element witch can be set via TOML file.
  bool ValidateConfig() const override;

  ///
  void MergeOutput(bool cleanUp=true) const;

  ///
  void DefineControlPoints();

  public:

  ///\brief Static method to get instance of this singleton object.
  static RunSvc *GetInstance();

  ///\brief Virtual method implementation defining the default units configuration.
  void DefaultConfig(const std::string &unit);

  ///\brief Set requested number of threads to be used in the simulation.
  void SetNofThreads(int val);

  /**
 * @brief Returns the list of Geant4 macro (.mac) files to be processed for the simulation run.
 *
 * @return Reference to a vector containing the paths of macro files.
 */
  inline const std::vector<G4String> &GetMacFiles() const { return m_macFiles; }

  /**
 * @brief Sets the operational mode of the application run.
 *
 * @param mode The desired operational mode (e.g., BuildGeometry or FullSimulation).
 */
  void AppMode(OperationalMode mode) { m_application_mode = mode; }

  ///\brief Perform service and global run related configuration initialization.
  void Initialize(WorldConstruction* world);

  ///\brief Perform service and global run related configuration finalization.
  void Finalize();

  ///\brief Main execution method
  void Run();

  ///\brief Build and export geometry into the gdml file
  void BuildGeometryMode();

  ///\brief Full simulation means specific physics and more realistic particle gun.
  /// Full simulation with electron gun, simulation of photons
  /// generation and propagation of photons and other particles until
  /// they stop at room floor.
  void FullSimulationMode();

  ///
  void RegisterRunComponent(RunComponet *element);

  ///
  void LoadSimulationPlan();

  /**
 * @brief Returns the pointer to the Geant4 run manager.
 *
 * @return G4RunManager* Pointer to the current Geant4 run manager instance managed by the service.
 */
  G4RunManager* G4RunManagerPtr() const { return m_g4RunManager; }


  /**
 * @brief Returns the list of defined control points for the simulation run.
 *
 * @return Reference to a vector containing all control points.
 */
const std::vector<ControlPoint>& GetControlPoints() const { return m_control_points; }

  /**
 * @brief Returns the current control point in the simulation.
 *
 * @return Pointer to the current ControlPoint, or nullptr if not set.
 */
  ControlPoint* CurrentControlPoint() const { return m_current_control_point; }

  /**
   * @brief Sets the current control point and returns it.
   *
   * @param cp Pointer to the control point to set as current.
   * @return ControlPoint* The control point that was set as current.
   */
  ControlPoint* CurrentControlPoint(ControlPoint* cp) { 
    // cp->FillPlanFieldMask();
    m_current_control_point = cp; 
    return cp; 
  }

  ///
  void ParseTomlConfig() override;

  ///
  std::string InitializeOutputDir();

  ///
  static std::string GetJobNameLabel(); 

  ///
  void WriteGeometryData() const;

  /**
 * @brief Returns the set of enabled scoring types for the simulation.
 *
 * @return Reference to the set of scoring types currently configured.
 */
  const std::set<Scoring::Type>& GetScoringTypes() const { return m_scoring_types; }
  /**
 * @brief Returns a reference to the set of enabled scoring types for the simulation.
 *
 * Allows modification of the scoring types used during the simulation run.
 *
 * @return Reference to the set of scoring types.
 */
std::set<Scoring::Type>& GetScoringTypes() { return m_scoring_types; }
};

#endif  // Dose3D_RUNSVC_H
