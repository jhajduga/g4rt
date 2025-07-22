# LogSvc Logging Service

## ✨ Introduction

**LogSvc** is a dedicated, high-performance logging service designed for large-scale simulation projects and scientific computing applications, including those based on Geant4. Leveraging the powerful [Loguru](https://github.com/emilk/loguru) library, LogSvc offers structured, modular logging, thread safety, dynamic verbosity adjustments, and easy integration with external logging streams like Geant4’s output streams (`G4cout`, `G4cerr`).

---

## 🚀 Features

- **Thread-Safe Logging:** Ensures thread safety using mutex locks, eliminating race conditions and ensuring log integrity.
- **Module-Specific Logs:** Allows creation of separate log files per module, enhancing clarity and simplifying debugging.
- **Dynamic Verbosity:** Adjust log verbosity at runtime without recompilation.
- **Integration with Geant4:** Seamlessly captures `G4cout` and `G4cerr` streams through a custom `G4UIsession` implementation.
- **Customizable Callbacks:** Supports adding user-defined callbacks for advanced logging scenarios (e.g., external monitoring systems, notifications).

---

## 🔧 Integration and Configuration

### Initialization

Initialize `LogSvc` at application startup to establish a centralized logging system:

```cpp
#include "LogSvc.hh"

int main(int argc, char* argv[]) {
    LogSvc::Init(argc, argv, "logs/simulation_main.log", loguru::Verbosity_MAX, 100);
    return 0;
}
```

Parameters:

- `argc`, `argv`: Command-line arguments.
- `default_log_file`: Default log file path.
- `verbosity`: Logging detail level (`loguru::Verbosity_INFO`, `loguru::Verbosity_ERROR`, etc.).
- `flush_interval_ms`: Interval for flushing logs to disk.

### Module-Specific Logging

To create individual log files for specific simulation modules:

```cpp
LogSvc::AddModuleLogFile("Physics", "logs/physics.log", loguru::Verbosity_ERROR);
LogSvc::AddModuleLogFile("Detector", "logs/detector.log", loguru::Verbosity_WARNING);
LogSvc::AddModuleLogFile("Analysis", "logs/analysis.log", loguru::Verbosity_INFO);
```

This modular logging simplifies log analysis and debugging.

---

## 🔄 Integration with Geant4 (G4cout and G4cerr)

To capture Geant4's `G4cout` and `G4cerr` streams into LogSvc, implement a custom session class deriving from `G4UIsession`:

```cpp
// LogSession.hh
#include "G4UIsession.hh"

class LogSession : public G4UIsession {
public:
    LogSession();
    virtual G4int ReceiveG4cout(const G4String& coutString);
    virtual G4int ReceiveG4cerr(const G4String& cerrString);
};
```

```cpp
// LogSession.cpp
#include "LogSession.hh"
#include "LogSvc.hh"

LogSession::LogSession() : G4UIsession() {
    auto UI = G4UImanager::GetUIpointer();
    UI->SetCoutDestination(this);
}

G4int LogSession::ReceiveG4cout(const G4String& coutString) {
    std::string msg = coutString;
    if (!msg.empty() && msg.back() == '\n') msg.pop_back();

    if (msg.find("[DEBUG]") != std::string::npos) {
        LOGSVC_DEBUG("G4Cout", "{}", msg);
    } else {
        LOGSVC_INFO("G4Cout", "{}", msg);
    }
    return 0;
}

G4int LogSession::ReceiveG4cerr(const G4String& cerrString) {
    std::string msg = cerrString;
    if (!msg.empty() && msg.back() == '\n') msg.pop_back();

    LOGSVC_ERROR("G4Cerr", "{}", msg);
    return 0;
}
```

Instantiate this session at application initialization to ensure all Geant4 outputs are logged:

```cpp
// main.cpp
#include "LogSession.hh"

int main(int argc, char* argv[]) {
    LogSvc::Init(argc, argv);

    // Setup LogSession for Geant4
    LogSession logSession;

    // Rest of Geant4 initialization...
}
```

---

## 📌 Usage Examples

Using convenient macros provided by LogSvc:

```cpp
LOGSVC_INFO("Physics", "Particle simulation started.");
LOGSVC_WARN("Detector", "Detector anomaly at energy: {} MeV", 13.37);
LOGSVC_ERROR("Analysis", "Data analysis failed due to missing inputs.");
```

Define custom macros for readability and module-specific logging:

```cpp
#define PHYSIC_INFO(msg, ...)    LOGSVC_INFO("PhysicsModule", msg, ##__VA_ARGS__)
#define DETECTOR_WARN(msg, ...)  LOGSVC_WARN("DetectorModule", msg, ##__VA_ARGS__)
#define ANALYSIS_ERROR(msg, ...) LOGSVC_ERROR("AnalysisModule", msg, ##__VA_ARGS__)

// Example Usage:
PHYSIC_INFO("Initialized particle beam with energy {} GeV.", 120.0);
DETECTOR_WARN("Sensor {} is reporting unusual values.", sensor_id);
ANALYSIS_ERROR("Failed to process input file '{}'.", input_file);
```

These macros enhance clarity and simplify logging calls.

---

## 🚦 Dynamic Verbosity

Adjust console logging verbosity dynamically at runtime:

```cpp
LogSvc::SetTerminalLogLevel(loguru::Verbosity_ERROR);
```

---

## ⚙️ Custom Callbacks

Custom callbacks allow developers to define special actions triggered by specific log events, enabling integration with external systems, such as monitoring dashboards, alerts, or event-driven frameworks.

Example of a custom callback to send critical errors to an external alerting service:

```cpp
loguru::add_callback(
    "external_alerts",
    [](void*, const loguru::Message& msg) {
        if (msg.verbosity <= loguru::Verbosity_ERROR) {
            sendCriticalAlert(msg.message);
        }
    },
    nullptr,
    loguru::Verbosity_ERROR
);
```

Another example is a callback triggered upon simulation completion to notify via email or a messaging service:

```cpp
loguru::add_callback(
    "simulation_complete_notifier",
    [](void*, const loguru::Message& msg) {
        if (msg.message.find("Simulation completed") != std::string::npos) {
            notifySimulationComplete(); // Can call for egample python script and send email about end of simulation to user. 
        }
    },
    nullptr,
    loguru::Verbosity_INFO
);
```

Callbacks can filter messages based on verbosity and perform custom logic beyond simple log writing.

---

## 📂 Example Log Outputs

**logs/physics.log:**

```bash
2025-03-21 09:00:01.123 (  0.001s) [main thread] physics.cpp:24      ERROR| Quantum chaos formula exceeded stability threshold!
```

**logs/G4Cout.log:**

```bash
2025-03-21 09:05:45.789 (345.667s) [main thread] G4Cout.cpp:12 INFO| Geant4: Simulation completed successfully.
```

**logs/G4Cerr.log:**

```bash
2025-03-21 09:07:21.543 (441.421s) [worker-thread] G4Cerr.cpp:30 ERROR| Geant4: Critical geometry overlap detected!
```

---

## 📑 Summary of Key Functions

| Function | Description |
|----------|-------------|
| `Init()` | Initializes the global logging service. |
| `AddModuleLogFile()` | Creates dedicated log files for individual modules. |
| `SetTerminalLogLevel()` | Dynamically adjusts the verbosity level of logs shown in the terminal. |
| `LOGSVC_INFO/WARNING/ERROR...` | Provides convenient macros for logging at different verbosity levels. |
| `loguru::add_callback()` | Registers custom callbacks for advanced logging scenarios (alerts, monitoring, etc.). |

---

## 🔑 Summary

LogSvc provides a robust, structured, and thread-safe logging mechanism specifically tailored for complex scientific applications. Its modular approach, dynamic verbosity adjustment, and seamless integration capabilities, including with external streams like Geant4’s `G4cout` and `G4cerr`, make LogSvc indispensable for simulation projects. Additionally, its extensible callback system empowers users to integrate logging seamlessly into broader monitoring, alerting, and event-driven architectures, significantly benefiting developers and researchers.

## TEMP

W tym momencie debug jest ustawiony w LogSvc jako najwyższy poziom logowania.
Ale możemy dodać inną systematykę w przyszłości.

### Proposal

| Nazwa   | Verbosity (int) | Opis                                                                 |
|---------|-----------------|----------------------------------------------------------------------|
| FATAL   | -3              | Krytyczny błąd, kończy program (ABORT_F/ABORT_S)                     |
| ERROR   | -2              | Błąd krytyczny, ale możliwy do przechwycenia                         |
| WARNING | -1              | Ostrzeżenie – potencjalny problem lub zachowanie niestandardowe       |
| INFO    | 0               | Standardowe logi użytkownika (starty, parametry, postępy)             |
| NOTICE  | +1              | Dodatkowe szczegóły (np. komunikaty o konfiguracji)                   |
| STATUS  | +2              | Regularne statusy, np. cykle symulacji, aktualizacja %                |
| DETAIL  | +3              | Szczegóły działania (np. który plik jest aktualnie przetwarzany)      |
| DEBUG   | +5              | Techniczne informacje pomocne przy debugowaniu                        |
| VERBOSE | +7              | Szczegółowe informacje dla zaawansowanych użytkowników                |
| TRACE   | +9 (MAX)        | Najniższy poziom – każdy szczegół, np. każda iteracja, zmienne        |

Ale to jakieś usprawnienie/dodatkowy feature na przyszłość.
(Temp Debug jest na 9 ustawiony...)
