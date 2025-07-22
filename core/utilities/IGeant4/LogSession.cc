#include "LogSession.hh"
#include "LogSvc.hh"
#include <sstream>

LogSession::LogSession() : G4UIsession() {
    auto UI = G4UImanager::GetUIpointer();
    UI->SetCoutDestination(this);
}

G4int LogSession::ReceiveG4cout(const G4String& coutString) {
    std::string msg = coutString;
    if (!msg.empty() && msg.back() == '\n') msg.pop_back();

    if (msg.find("[DEBUG]") != std::string::npos) {
        LOGSVC_DEBUG_RAW("G4Cout", "{}", msg);
    } else {
        LOGSVC_INFO_RAW("G4Cout", "{}", msg);
    }
    return 0;
}

G4int LogSession::ReceiveG4cerr(const G4String& cerrString) {
    std::string msg = cerrString;
    if (!msg.empty() && msg.back() == '\n') msg.pop_back();

    LOGSVC_ERROR_RAW("G4Cerr", "{}", msg);
    return 0;
}
