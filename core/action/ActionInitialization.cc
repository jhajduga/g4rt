#include "ActionInitialization.hh"
#include "PrimaryGenerationAction.hh"
#include "G4ScoringManager.hh"
#include "UserScoreWriter.hh"
#include "EventAction.hh"
#include "SteppingAction.hh"
#include "RunAction.hh"
#include "LogSvc.hh"
#include "G4Threading.hh"
/////////////////////////////////////////////////////////////////////////////
/**
 * @brief Configures user actions for the master thread in a multithreaded Geant4 simulation.
 *
 * Sets a custom score writer and registers the master thread's run-level action. This setup is specific to the master thread and may differ from worker thread configurations.
 */
void ActionInitialization::BuildForMaster() const {
  // In MT mode, to be clearer, the RunAction class for the master thread might be
  // different than the one used for the workers.
  // This RunAction will be called before and after starting the workers.
  // For more details, please refer to :
  // https://twiki.cern.ch/twiki/bin/view/Geant4/Geant4MTForApplicationDevelopers
  //

  G4ScoringManager::GetScoringManager()->SetScoreWriter(new UserScoreWriter());

  SetUserAction(new RunAction());
}

/////////////////////////////////////////////////////////////////////////////
/**
 * @brief Configures user actions for a worker thread in a Geant4 simulation.
 *
 * Sets the thread name, assigns a custom score writer, and registers user actions for primary particle generation, run-level, event-level, and step-level processing. The event and stepping actions are linked to enable event-specific step handling.
 */
void ActionInitialization::Build() const {

  std::stringstream name;
  name << "worker-" << G4Threading::G4GetThreadId();
  LogSvc::SetThreadName(std::string("worker-" + std::to_string(G4Threading::G4GetThreadId())));  

  G4ScoringManager::GetScoringManager()->SetScoreWriter(new UserScoreWriter());

  // Initialize the primary particles
  SetUserAction(new PrimaryGenerationAction());

  // Optional UserActions: run, event, stepping
  SetUserAction(new RunAction());
  auto evt = new EventAction();
  SetUserAction(evt);
  SetUserAction(new SteppingAction(evt));

}