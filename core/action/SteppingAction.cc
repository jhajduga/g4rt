#include "SteppingAction.hh"
#include "EventAction.hh"
#include "G4RunManager.hh"
#include "G4SteppingManager.hh"
#include "G4VProcess.hh"
#include "G4UnitsTable.hh"


/////////////////////////////////////////////////////////////////////////////
///
SteppingAction::SteppingAction(EventAction* eventAct):G4UserSteppingAction(), m_EventAction(eventAct){}

/////////////////////////////////////////////////////////////////////////////
///
void SteppingAction::UserSteppingAction(const G4Step* aStep) {
  // ?
  // G4int stepNb = aStep->GetTrack()->GetCurrentStepNumber();
  // G4StepPoint* postPoint = aStep->GetPostStepPoint();
  // auto position = postPoint->GetPosition();
  //G4cout << "\n  [debug]:: step " << stepNb << "   position  " << position << G4endl;

}




