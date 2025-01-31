#include "LogSession.hh"
#include <iostream>


LogSession::LogSession():G4UIsession() {
// LogSession::LogSession():G4UIsession(),Logable("G4Cout") {
    // auto UI = G4UImanager::GetUIpointer();
    // UI->SetCoutDestination(this);
};

G4int LogSession::ReceiveG4cout(const G4String& coutString) {
//    std::cout << "!!!MySeesin!!!!" << coutString << std::flush;
    auto msg = (std::ostringstream{} << coutString).str();
    msg.pop_back();
    if (msg.find("[DEBUG]") != std::string::npos) {
        // LOGSVC_DEBUG("{}", msg);
    }
    else {
        // LOGSVC_INFO("{}", msg);
    }
    return 0;
};
G4int LogSession::ReceiveG4cerr(const G4String& cerrString) {
    // std::cout << "!!!MySeesinErr!!!!" << cerrString << std::flush;
    auto msg = (std::ostringstream{} << cerrString).str();
    msg.pop_back();
    // LOGSVC_ERROR("{}", msg);
    return 0;
};