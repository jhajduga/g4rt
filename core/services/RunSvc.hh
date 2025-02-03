/**
*
* \author B.Rachwal (brachwal@agh.edu.pl)
* \date 10.01.2018
*
*/

#ifndef Dose3D_RUNSVC_H
#define Dose3D_RUNSVC_H

// #include "Logable.hh"
#include "TomlConfigurable.hh"
#include "Types.hh"
#include "G4RunManager.hh"
#include "ControlPoint.hh"
#include "VoxelHit.hh"
#include "logger.hh"

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
// class RunSvc : public TomlConfigurable, Logable {
class RunSvc : public TomlConfigurable {
  private:
  
  // Nadpisanie (przedefiniowanie) globalnych makr logujących na modułowe w pliku RunSvc.

  // #define RUNSVC_DEBUG(msg, ...) LOG_TO_MODULE("RunSvc", loguru::Verbosity_MAX, msg, ##__VA_ARGS__)
  // #define RUNSVC_INFO(msg, ...)  LOG_TO_MODULE("RunSvc", loguru::Verbosity_INFO, msg, ##__VA_ARGS__)
  // #define RUNSVC_WARNING(msg, ...)  LOG_TO_MODULE("RunSvc", loguru::Verbosity_WARNING, msg, ##__VA_ARGS__)
  // #define RUNSVC_ERROR(msg, ...)  LOG_TO_MODULE("RunSvc", loguru::Verbosity_ERROR, msg, ##__VA_ARGS__)
  // #define RUNSVC_FATAL(msg, ...)  LOG_TO_MODULE("RunSvc", loguru::Verbosity_FATAL, msg, ##__VA_ARGS__)

  RunSvc();

  ~RunSvc();

  // Delete the copy and move constructors
  RunSvc(const RunSvc &) = delete;

  RunSvc &operator=(const RunSvc &) = delete;

  RunSvc(RunSvc &&) = delete;

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

  ///\brief Get the corresponding .mac files container
  inline const std::vector<G4String> &GetMacFiles() const { return m_macFiles; }

  ///\brief Set an appropriate application run-mode.
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

  ///
  G4RunManager* G4RunManagerPtr() const { return m_g4RunManager; }


  const std::vector<ControlPoint>& GetControlPoints() const { return m_control_points; }

  ///
  ControlPoint* CurrentControlPoint() const { return m_current_control_point; }

  ///
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

  ///
  const std::set<Scoring::Type>& GetScoringTypes() const { return m_scoring_types; }
  std::set<Scoring::Type>& GetScoringTypes() { return m_scoring_types; }
};

#endif  // Dose3D_RUNSVC_H
