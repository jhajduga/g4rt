#include "RunAction.hh"
#include "G4Run.hh"
#include "Services.hh"
#include "WorldConstruction.hh"
#include "SavePhSpAnalysis.hh"
#include "RunAnalysis.hh"
#include "BeamAnalysis.hh"
#include "PrimariesAnalysis.hh"
#include "StepAnalysis.hh"
#include "NTupleEventAnalisys.hh"
#include "colors.hh"
#include <map>
#include <fstream>
#include <iostream>

// NOTE:
// It is recommended, but not necessary, to create the analysis manager in the user
// run action constructor and delete it in its destructor. This guarantees correct
// behavior in multi-threading mode.
/////////////////////////////////////////////////////////////////////////////
/**
 * @brief Constructs a RunAction object and configures analysis settings for the simulation run.
 *
 * Initializes the analysis manager singleton, sets ntuple merging and verbosity based on configuration, and enables run scoring if specified.
 */
RunAction::RunAction() : G4UserRunAction(), fAnalysisManager(nullptr) {
  fAnalysisManager = G4AnalysisManager::Instance();

  if (Service<ConfigSvc>()->GetValue<bool>("RunSvc", "NTupleAnalysis"))
    fAnalysisManager->SetNtupleMerging(false);  
  fAnalysisManager->SetVerboseLevel(0);
  if (Service<ConfigSvc>()->GetValue<bool>("RunSvc", "RunAnalysis"))
    m_run_scoring = true;
}

/////////////////////////////////////////////////////////////////////////////
/**
 * @brief Destructor for RunAction, cleaning up the analysis manager on the master thread.
 *
 * Deletes the analysis manager singleton and resets its pointer to null if called on the master thread.
 */
RunAction::~RunAction(){
  if (IsMaster()) {
    delete fAnalysisManager;
    fAnalysisManager = nullptr;
  }

}

/////////////////////////////////////////////////////////////////////////////
/**
 * @brief Generates and returns a new simulation run using the current control point.
 *
 * If called on the master thread, prints informational messages about the new run, including the number of events and any defined rotation. Delegates run creation to the current control point, passing the run scoring flag.
 *
 * @return G4Run* Pointer to the newly generated run.
 */
G4Run* RunAction::GenerateRun(){
  auto control_point = Service<RunSvc>()->CurrentControlPoint();
  if (IsMaster()){
    // control_point->InitializeRun();
    G4cout << FGRN("[INFO]") << ":: " << FBLU("GENERATING NEW RUN... ") << G4endl;
    G4cout << FGRN("[INFO]") << ":: NEvents: " << control_point->GetNEvts() << G4endl;
    auto rot = control_point->GetRotation();
    if (rot)
      G4cout << FGRN("[INFO]") << ":: Rotation: " << *control_point->GetRotation() << G4endl;
  }
  return control_point->GenerateRun(m_run_scoring);
}
/////////////////////////////////////////////////////////////////////////////
/**
 * @brief Prepares analysis modules and output files at the start of a simulation run.
 *
 * Opens the analysis output file, prints run start information distinguishing master and worker threads, and initializes enabled analysis modules based on configuration. If running as master, outputs geometry information. Starts the internal run timer.
 *
 * @param aRun Pointer to the current Geant4 run.
 */
void RunAction::BeginOfRunAction(const G4Run* aRun) {
  auto configSvc = Service<ConfigSvc>();
  auto runSvc = Service<RunSvc>();

  fAnalysisManager->SetFileName(runSvc->CurrentControlPoint()->GetSimOutputTFileName(true));
  fAnalysisManager->OpenFile();

  if (IsMaster())
    G4cout << "### Run " << aRun->GetRunID() << " starts (master)." << G4endl;
  else
    G4cout << "### Run " << aRun->GetRunID() << " starts (worker)." << G4endl;

  //___________________________________________________________________________
  // TODO:: This is not revised yet,
  // The SavePhSpAnalysis::FillPhSp is not finished
  if (configSvc->GetValue<bool>("RunSvc", "SavePhSp")) {
    SavePhSpAnalysis::GetInstance()->BeginOfRun(aRun, IsMaster());
  }

  //___________________________________________________________________________
  if (configSvc->GetValue<bool>("RunSvc", "RunAnalysis"))
    RunAnalysis::GetInstance()->BeginOfRun(aRun, IsMaster());

  if (configSvc->GetValue<bool>("RunSvc", "PrimariesAnalysis"))
    PrimariesAnalysis::GetInstance()->BeginOfRun(aRun, IsMaster());

  if (configSvc->GetValue<bool>("RunSvc", "BeamAnalysis"))
    BeamAnalysis::GetInstance()->BeginOfRun(aRun, IsMaster());

  if (configSvc->GetValue<bool>("RunSvc", "StepAnalysis"))
    StepAnalysis::GetInstance()->BeginOfRun(aRun, IsMaster());

  if (configSvc->GetValue<bool>("RunSvc", "NTupleAnalysis") && NTupleEventAnalisys::IsAnyTTreeDefined() ) 
    NTupleEventAnalisys::GetInstance()->BeginOfRun(aRun, IsMaster());

  //___________________________________________________________________________
  // Setup geometry configuration and write it's information to the screen
  if (IsMaster())
    Service<GeoSvc>()->World()->WriteInfo();

  m_timer.Start();
}

/////////////////////////////////////////////////////////////////////////////
/**
 * @brief Finalizes the simulation run, writes analysis data, and reports elapsed time.
 *
 * Stops the internal timer, prints the elapsed time for the run (distinguishing between master and worker threads), writes and closes the analysis output file, and, if enabled and on the master thread, finalizes run-level analysis.
 */
void RunAction::EndOfRunAction(const G4Run* aRun) {
  m_timer.Stop();
  G4double loopRealElapsedTime = m_timer.GetRealElapsed();
  if (IsMaster()) {
    G4cout << "Global-loop elapsed time [s] : " << loopRealElapsedTime << G4endl;
  }
  else
    G4cout << "Local-loop elapsed time [s] : " << loopRealElapsedTime << G4endl;

  fAnalysisManager->Write();
  fAnalysisManager->CloseFile();

  //___________________________________________________________________________
  auto configSvc = Service<ConfigSvc>();
  if(configSvc->GetValue<bool>("RunSvc", "RunAnalysis") && IsMaster())
    RunAnalysis::GetInstance()->EndOfRun(aRun);

  // Service<RunSvc>()->EndOfRun();
}
