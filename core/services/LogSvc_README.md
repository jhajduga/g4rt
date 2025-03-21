# LogSvc Logging Service

## ✨ **Introduction to LogSvc**

LogSvc is an advanced, dedicated logging service designed specifically for integration within large-scale simulation projects, including applications built on frameworks like Geant4. Utilizing the powerful [Loguru](https://github.com/emilk/loguru) library, LogSvc provides a flexible and efficient mechanism for managing logs across different modules, supports multiple logging levels, and automates the process of log flushing to ensure timely persistence.

---

## 📁 **Integration and Configuration in Simulation Projects**

### **1. Initialization of Logging Service**

In the main simulation project, LogSvc is initialized at the application's startup, ensuring consistent and centralized logging throughout the runtime:

```cpp
#include "LogSvc.hpp"

int main(int argc, char* argv[]) {
    LogSvc::Init(argc, argv, "logs/simulation_main.log", loguru::Verbosity_MAX, 100);
    return 0;
}
```

- **Initialization Parameters:**
  - `argc, argv` – standard command-line arguments passed to the application.
  - `default_log_file` – default log file for the entire simulation.
  - `verbosity` – sets the logging verbosity (detail level).
  - `flush_interval_ms` – frequency of log flushing (writing logs to disk).

---

### **2. Module-specific Logging Configuration**

Each module within the simulation environment (such as physics, detector, and data analysis modules) can have its own dedicated log file. This modular approach significantly enhances clarity and simplifies troubleshooting:

```cpp
LogSvc::AddModuleLogFile("Physics", "logs/physics.log", loguru::Verbosity_ERROR);
LogSvc::AddModuleLogFile("Detector", "logs/detector.log", loguru::Verbosity_WARNING);
LogSvc::AddModuleLogFile("Analysis", "logs/analysis.log", loguru::Verbosity_INFO);
```

- **AddModuleLogFile()** – enables separate logs for each module, each with individually customizable verbosity levels.

---

## 🔧 **Logging Functions**

LogSvc provides a convenient set of macros for quickly generating structured log messages within the simulation project:

```cpp
LOGSVC_INFO("Physics", "Particle simulation started.");
LOGSVC_WARNING("Detector", "Anomaly detected at energy level: {} MeV", 13.37);
LOGSVC_ERROR("Analysis", "Data input analysis failed.");
```

These macros simplify logging and maintain consistency across the codebase.

---

## 🚀 **Defining Local Logging Macros for Modules**

To further streamline and simplify logging within specific modules, you can define local macros. These improve readability and allow quick adjustment of the target module for logs:

```cpp
#define DEFAULT_MODULE "Physics"

#define LOG_INFO(msg, ...)    LOGSVC_INFO(DEFAULT_MODULE, msg, ##__VA_ARGS__)
#define LOG_WARN(msg, ...)    LOGSVC_WARNING(DEFAULT_MODULE, msg, ##__VA_ARGS__)
#define LOG_ERROR(msg, ...)   LOGSVC_ERROR(DEFAULT_MODULE, msg, ##__VA_ARGS__)
```

Such definitions ensure module-specific logs remain concise and highly readable.

---

## 📌 **Example Logs from a Simulation Run**

**File: logs/physics.log:**
```
2025-03-21 09:00:01.123 (  0.001s) [main thread] physics.cpp:24      ERROR| Quantum chaos formula exceeded stability threshold!
```

**File: logs/detector.log:**
```
2025-03-21 09:01:15.567 ( 14.444s) [worker-thread] detector.cpp:87 WARN| Detector registered anomaly at energy level: 13.37 MeV
```

---

## 🔒 **Dynamic Adjustment of Logging Levels**

During runtime, LogSvc supports dynamic changes to the console log verbosity, enabling adjustments to the level of detail displayed:

```cpp
LogSvc::SetTerminalLogLevel(loguru::Verbosity_ERROR);
```

---

## 🔄 **Custom Event Handling via Callbacks**

LogSvc also allows the addition of custom callback functions for special-purpose log handling, such as external monitoring systems:

```cpp
loguru::add_callback(
    "external_monitor",
    [](void* user_data, const loguru::Message& message) {
        sendAlertToMonitoringSystem(message.message);
    },
    nullptr,
    loguru::Verbosity_WARNING
);
```

---

## 📈 **Summary of Key Functions**

| Function                     | Role in Simulation Project          |
|------------------------------|-------------------------------------|
| `Init()`                     | Initialize global logging service   |
| `AddModuleLogFile()`         | Assign separate log files per module|
| `SetTerminalLogLevel()`      | Adjust terminal logging verbosity   |
| `LOGSVC_DEBUG/INFO/ERROR...` | User-friendly logging macros        |
| `add_callback()`             | Customizable log handling via callback|

---

✨ **LogSvc** seamlessly integrates a robust, structured logging system into large-scale simulation projects, significantly enhancing maintainability, debugging capability, and overall project clarity, particularly in sophisticated scientific computing environments.

