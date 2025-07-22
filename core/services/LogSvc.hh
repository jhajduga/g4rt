// LogSvc.hh
#pragma once
#include <loguru.hpp>
#include <fmt/format.h>
#include "FmtFormatters.hh"
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>


class LogSvc {
public:

    /**
     * @brief Initializes the logging service.
     * @param argc Argument count from main.
     * @param argv Argument values from main.
     * @param default_log_file Path to the default log file.
     * @param verbosity Logging verbosity level.
     * @param flush_interval_ms Interval in milliseconds for flushing logs.
     */
    static void Init(int argc, const char* argv[], const std::string& default_log_file = "app.log",
                     loguru::Verbosity verbosity = loguru::Verbosity_INFO, int flush_interval_ms = 100);
    
    
                     /**
     * @brief Adds a new module-specific log file.
     * @param module Name of the module.
     * @param log_file Path to the log file for the module.
     * @param verbosity Logging verbosity level.
     */
    static void AddModuleLogFile(const std::string& module, const std::string& full_log_path, loguru::Verbosity verbosity);
    static void SetThreadName(const std::string& name);
    static loguru::Verbosity ParseVerbosityLevel(const std::string& level_str);

    /**
     * @brief Ustawia poziom logowania do terminala.
     * @param verbosity Poziom logowania (np. loguru::Verbosity_INFO, loguru::Verbosity_ERROR).
     */
    static void SetTerminalLogLevel(loguru::Verbosity verbosity);

    static void SetLogFolder(const std::string& folder) { s_log_folder = folder; }
    static std::string GetLogFolder() { return s_log_folder; }


    static loguru::Verbosity GetVerbosity() { return s_terminal_verbosity; }

    /**
     * @brief Replaces the current main log file with a new one.
     * @param new_log_full_path Full path to the new log file.
     */
    static void ReconfigureMainLog(const std::string& new_log_full_path);
    static void EnableCustomVerbosityNames();
    static void EnableColoredTerminalOutput();

    

    /**
     * @brief Logs a message with debug verbosity for a given module.
     */
    template<typename... Args>
    static void LogDebug(const std::string& module, const char* file, int line, const char* format, const Args&... args) {
        logToModule(module, loguru::Verbosity_9, file, line, format, args...);
    }

    template<typename... Args>
    static void LogInfo(const std::string& module, const char* file, int line, const char* format, const Args&... args) {
        logToModule(module, loguru::Verbosity_INFO, file, line, format, args...);
    }

    template<typename... Args>
    static void LogWarning(const std::string& module, const char* file, int line, const char* format, const Args&... args) {
        logToModule(module, loguru::Verbosity_WARNING, file, line, format, args...);
    }

    template<typename... Args>
    static void LogError(const std::string& module, const char* file, int line, const char* format, const Args&... args) {
        logToModule(module, loguru::Verbosity_ERROR, file, line, format, args...);
    }

    template<typename... Args>
    static void LogFatal(const std::string& module, const char* file, int line, const char* format, const Args&... args) {
        logToModule(module, loguru::Verbosity_FATAL, file, line, format, args...);
        loguru::flush();
    }
private:
    static inline std::mutex module_log_files_mutex;

    static inline std::string s_log_folder = "logs";
    static inline std::string s_main_log_id = "";
    static inline loguru::Verbosity s_terminal_verbosity = loguru::Verbosity_INFO;


    static std::unordered_map<std::string, std::shared_ptr<FILE>> module_log_files;
    /**
     * @brief Logs a formatted message to the specified module.
     */


template<typename... Args>
static void logToModule(const std::string& module, loguru::Verbosity verbosity, const char* file, int line, const char* format, const Args&... args) {
    std::string formatted_message = fmt::vformat(format, fmt::make_format_args(args...));
    loguru::log(verbosity, file, line, "[{}] {}", module, formatted_message);
}
};

#define DEFAULT_MODULE  "Main"
#define SERVICE_MODULE  "Service"
#define GEOMETRY_MODULE "Geometry"
#define ANALYSIS_MODULE "Analysis"

#define LOGSVC_DEBUG( module,       msg, ...)       LogSvc::LogDebug(   module, __FILE__, __LINE__, msg, ##__VA_ARGS__)
#define LOGSVC_INFO(  module,       msg, ...)       LogSvc::LogInfo(    module, __FILE__, __LINE__, msg, ##__VA_ARGS__)
#define LOGSVC_WARN(  module,       msg, ...)       LogSvc::LogWarning( module, __FILE__, __LINE__, msg, ##__VA_ARGS__)
#define LOGSVC_ERROR( module,       msg, ...)       LogSvc::LogError(   module, __FILE__, __LINE__, msg, ##__VA_ARGS__)
#define LOGSVC_FATAL( module,       msg, ...)       LogSvc::LogFatal(   module, __FILE__, __LINE__, msg, ##__VA_ARGS__)

#define LOGSVC_DEBUG_RAW( module,   msg, ...)       LogSvc::LogDebug(   module, "Geant4",        0, msg, ##__VA_ARGS__)
#define LOGSVC_INFO_RAW(  module,   msg, ...)       LogSvc::LogInfo(    module, "Geant4",        0, msg, ##__VA_ARGS__)
#define LOGSVC_WARN_RAW(  module,   msg, ...)       LogSvc::LogWarning( module, "Geant4",        0, msg, ##__VA_ARGS__)
#define LOGSVC_ERROR_RAW( module,   msg, ...)       LogSvc::LogError(   module, "Geant4",        0, msg, ##__VA_ARGS__)
#define LOGSVC_FATAL_RAW( module,   msg, ...)       LogSvc::LogFatal(   module, "Geant4",        0, msg, ##__VA_ARGS__)

#define LOG_DEBUG(                  msg, ...)       LOGSVC_DEBUG(       DEFAULT_MODULE,             msg, ##__VA_ARGS__)
#define LOG_INFO(                   msg, ...)       LOGSVC_INFO(        DEFAULT_MODULE,             msg, ##__VA_ARGS__)
#define LOG_WARN(                   msg, ...)       LOGSVC_WARN(        DEFAULT_MODULE,             msg, ##__VA_ARGS__)
#define LOG_ERROR(                  msg, ...)       LOGSVC_ERROR(       DEFAULT_MODULE,             msg, ##__VA_ARGS__)
#define LOG_FATAL(                  msg, ...)       LOGSVC_FATAL(       DEFAULT_MODULE,             msg, ##__VA_ARGS__)

#define ANA_DEBUG(                  msg, ...)       LOGSVC_DEBUG(       ANALYSIS_MODULE,            msg, ##__VA_ARGS__)
#define ANA_INFO(                   msg, ...)       LOGSVC_INFO(        ANALYSIS_MODULE,            msg, ##__VA_ARGS__)
#define ANA_WARN(                   msg, ...)       LOGSVC_WARN(        ANALYSIS_MODULE,            msg, ##__VA_ARGS__)
#define ANA_ERROR(                  msg, ...)       LOGSVC_ERROR(       ANALYSIS_MODULE,            msg, ##__VA_ARGS__)
#define ANA_FATAL(                  msg, ...)       LOGSVC_FATAL(       ANALYSIS_MODULE,            msg, ##__VA_ARGS__)

#define DEBUG_SVC(                  msg, ...)       LOGSVC_DEBUG(       SERVICE_MODULE,             msg, ##__VA_ARGS__)
#define INFO_SVC(                   msg, ...)       LOGSVC_INFO(        SERVICE_MODULE,             msg, ##__VA_ARGS__)
#define WARN_SVC(                   msg, ...)       LOGSVC_WARN(        SERVICE_MODULE,             msg, ##__VA_ARGS__)
#define ERROR_SVC(                  msg, ...)       LOGSVC_ERROR(       SERVICE_MODULE,             msg, ##__VA_ARGS__)
#define FATAL_SVC(                  msg, ...)       LOGSVC_FATAL(       SERVICE_MODULE,             msg, ##__VA_ARGS__)

#define DEBUG_GEO(                  msg, ...)       LOGSVC_DEBUG(       GEOMETRY_MODULE,            msg, ##__VA_ARGS__)
#define INFO_GEO(                   msg, ...)       LOGSVC_INFO(        GEOMETRY_MODULE,            msg, ##__VA_ARGS__)
#define WARN_GEO(                   msg, ...)       LOGSVC_WARN(        GEOMETRY_MODULE,            msg, ##__VA_ARGS__)
#define ERROR_GEO(                  msg, ...)       LOGSVC_ERROR(       GEOMETRY_MODULE,            msg, ##__VA_ARGS__)
#define FATAL_GEO(                  msg, ...)       LOGSVC_FATAL(       GEOMETRY_MODULE,            msg, ##__VA_ARGS__)


