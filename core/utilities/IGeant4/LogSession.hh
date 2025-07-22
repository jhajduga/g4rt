
#ifndef Dose3D_LOGSESSION_H
#define Dose3D_LOGSESSION_H
#include "G4UImanager.hh"
#include "G4UIsession.hh"

/**
 * @brief Klasa sesji UI dla Geant4, przechwytująca G4cout i G4cerr
 *        i kierująca je do LogSvc.
 */
class LogSession : public G4UIsession {
public:
    LogSession();

    G4int ReceiveG4cout(const G4String& coutString) override;
    G4int ReceiveG4cerr(const G4String& cerrString) override;
};
#endif

