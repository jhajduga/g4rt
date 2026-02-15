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
 * @brief Initialize run-action and configure the Geant4 analysis manager.
 *
 * Acquires the G4AnalysisManager singleton, disables ntuple merging when NTupleAnalysis
 * is enabled in RunSvc, sets the analysis manager verbosity to 0, and enables
 * run-level scoring when RunAnalysis is enabled.
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
 * @brief Release the Geant4 analysis manager when running on the master thread.
 *
 * Deletes the analysis manager singleton and clears its pointer if invoked on the master thread.
 */
RunAction::~RunAction(){
  if (IsMaster()) {
    delete fAnalysisManager;
    fAnalysisManager = nullptr;
  }

}

/////////////////////////////////////////////////////////////////////////////
/**
 * @brief Create a new G4Run using the current RunSvc control point.
 *
 * Retrieves the current control point from RunSvc and delegates run creation to it,
 * passing the internal run-scoring flag. When invoked on the master thread, emits
 * informational messages about the run (number of events and, if present, rotation).
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
 * @brief Prepare and initialize analyses and output for a new Geant4 run.
 *
 * Sets the analysis manager output file and opens it, emits a master/worker run-start message,
 * invokes BeginOfRun on each enabled analysis module (SavePhSpAnalysis, RunAnalysis,
 * PrimariesAnalysis, BeamAnalysis, StepAnalysis, and NTupleEventAnalisys when applicable),
 * writes geometry information when running on the master thread, and starts the internal run timer.
 *
 * @param aRun Pointer to the current Geant4 run used for run identification and passed to analyses.
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
 * @brief Finalize a run: stop timing, write/close analysis output, and finish run-level analyses.
 *
 * Stops the internal run timer and prints the real elapsed time (reports "Global-loop" on the master
 * thread and "Local-loop" on workers). Writes and closes the analysis manager's output file. If
 * run-level analysis is enabled in RunSvc and executing on the master thread, invokes
 * RunAnalysis::EndOfRun for finalization.
 *
 * @param aRun Pointer to the completed G4Run (passed to RunAnalysis::EndOfRun when invoked).
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