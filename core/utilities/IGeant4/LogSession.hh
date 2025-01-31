/**
*
* \author P.Matejski (pmatejski@agh.edu.pl)
* \date 05.06.2023
*
*/

//#ifndef Dose3D_LOGSESSION_H
//#define Dose3D_LOGSESSION_H

// #include "Logable.hh"
#include "G4UIsession.hh"


// class LogSession : public G4UIsession, Logable {
class LogSession : public G4UIsession {
    public:
    LogSession();
    G4int ReceiveG4cout(const G4String& coutString) ;
    G4int ReceiveG4cerr(const G4String& cerrString) ; 
};

//#endif

