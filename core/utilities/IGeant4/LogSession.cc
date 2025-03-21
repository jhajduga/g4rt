#include "LogSession.hh"
#include <iostream>


/**
 * @brief Constructs a LogSession instance.
 *
 * Initializes the LogSession as a G4UIsession without setting it as an output destination.
 * Note that the registration with the UI manager and any logging functionality has been disabled.
 */
LogSession::LogSession():G4UIsession() {
// LogSession::LogSession():G4UIsession(),Logable("G4Cout") {
    // auto UI = G4UImanager::GetUIpointer();
    // UI->SetCoutDestination(this);
};

/**
 * @brief Processes Geant4 standard output messages.
 *
 * This function converts the provided Geant4 output message into a standard string,
 * removes its trailing character, and checks for a "[DEBUG]" flag to determine the log level.
 * If "[DEBUG]" is present, the message is intended for debug logging; otherwise, for informational logging.
 * Note that the logging calls are currently disabled.
 *
 * @param coutString Geant4 standard output message.
 * @return G4int Always returns 0.
 */
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
/**
 * @brief Processes a Geant4 error message.
 *
 * Converts the provided error string into a standard string, removes its trailing character (typically a newline),
 * and is set up to log the processed error message (logging is currently disabled).
 *
 * @param cerrString The error message received from Geant4.
 * @return G4int Always returns 0.
 */
G4int LogSession::ReceiveG4cerr(const G4String& cerrString) {
    // std::cout << "!!!MySeesinErr!!!!" << cerrString << std::flush;
    auto msg = (std::ostringstream{} << cerrString).str();
    msg.pop_back();
    // LOGSVC_ERROR("{}", msg);
    return 0;
};