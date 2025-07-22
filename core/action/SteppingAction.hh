/**
*
* \author B. Rachwal
* \date 18.09.2021
*
*/

#ifndef Dose3D_STEPPING_ACTION_HH
#define Dose3D_STEPPING_ACTION_HH

#include "G4UserSteppingAction.hh"

class EventAction;

class SteppingAction : public G4UserSteppingAction{
  private:
    ///
    EventAction*  m_EventAction;

  public:
    ///
    SteppingAction(EventAction* eventAct);

    ///
    ~SteppingAction() = default;
    
    ///
    void UserSteppingAction(const G4Step*) override;
};


#endif // Dose3D_STEPPING_ACTION_HH
