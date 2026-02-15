#include "SteppingAction.hh"
#include "EventAction.hh"
#include "G4RunManager.hh"
#include "G4SteppingManager.hh"
#include "G4VProcess.hh"
#include "G4UnitsTable.hh"


/////////////////////////////////////////////////////////////////////////////
/**
 * @brief Create a SteppingAction associated with a specific EventAction.
 *
 * @param eventAct Pointer to the EventAction that this SteppingAction will use during stepping callbacks; ownership is not transferred.
 */
SteppingAction::SteppingAction(EventAction* eventAct):G4UserSteppingAction(), m_EventAction(eventAct){}

/////////////////////////////////////////////////////////////////////////////
/**
 * @brief Called at each simulation step to process step-specific actions.
 *
 * This method is invoked by the simulation framework for every step taken by a particle. Currently, it does not perform any operations.
 *
 * @param aStep Pointer to the current simulation step.
 */
void SteppingAction::UserSteppingAction(const G4Step* aStep) {
  // ?
  // G4int stepNb = aStep->GetTrack()->GetCurrentStepNumber();
  // G4StepPoint* postPoint = aStep->GetPostStepPoint();
  // auto position = postPoint->GetPosition();
  //G4cout << "\n  [debug]:: step " << stepNb << "   position  " << position << G4endl;

}



