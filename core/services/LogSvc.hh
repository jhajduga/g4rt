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

    /**
 * @brief Set the folder path used for storing log files.
 *
 * @param folder Path to the directory where new log files will be created; replaces the previously configured log folder.
 */
static void SetLogFolder(const std::string& folder) { s_log_folder = folder; }
    /**
 * @brief Get the current log folder path used for storing log files.
 *
 * @return std::string The path to the directory where log files are stored.
 */
static std::string GetLogFolder() { return s_log_folder; }


    /**
 * @brief Retrieves the current verbosity level configured for terminal output.
 *
 * @return loguru::Verbosity The verbosity level used for terminal logs.
 */
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
    /**
     * @brief Logs a debug-level message for the specified module.
     *
     * Formats and logs a message at verbosity level 9 (debug) for the given module, including source file and line information.
     */
    static void LogDebug(const std::string& module, const char* file, int line, const char* format, const Args&... args) {
        logToModule(module, loguru::Verbosity_9, file, line, format, args...);
    }

    template<typename... Args>
    /**
     * @brief Logs an informational message for a specified module with formatting and source location.
     *
     * Formats the message using the provided format string and arguments, and logs it at info verbosity level, tagging it with the module name, source file, and line number.
     */
    static void LogInfo(const std::string& module, const char* file, int line, const char* format, const Args&... args) {
        logToModule(module, loguru::Verbosity_INFO, file, line, format, args...);
    }

    template<typename... Args>
    /**
     * @brief Logs a warning-level message for the specified module including source location.
     *
     * The message is produced by formatting `format` with the provided `args` and is emitted with
     * warning verbosity, prefixed or tagged by `module` and annotated with `file` and `line`.
     *
     * @tparam Args Variadic types used to format the message.
     * @param module Module name used as the log tag.
     * @param file Source file path to associate with the log record.
     * @param line Source line number to associate with the log record.
     * @param format Format string used to produce the log message.
     * @param args Values referenced by `format` to produce the final message.
     */
    static void LogWarning(const std::string& module, const char* file, int line, const char* format, const Args&... args) {
        logToModule(module, loguru::Verbosity_WARNING, file, line, format, args...);
    }

    template<typename... Args>
    /**
     * @brief Logs a formatted error-level message for the specified module.
     *
     * Logs a message at error verbosity, including the module name, source file, and line number. The message is formatted using the provided format string and arguments.
     */
    static void LogError(const std::string& module, const char* file, int line, const char* format, const Args&... args) {
        logToModule(module, loguru::Verbosity_ERROR, file, line, format, args...);
    }

    template<typename... Args>
    /**
     * @brief Logs a formatted fatal-level message for the specified module and immediately flushes all logs.
     *
     * The message is tagged with the module name, source file, and line number. This method is intended for unrecoverable errors and ensures that all log output is written to disk before returning.
     */
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
/**
 * @brief Log a message tagged with a module name at the specified verbosity and source location.
 *
 * The message is constructed from a `fmt`-style format string and its arguments, then emitted to the logging backend
 * with the module name prefixed.
 *
 * @tparam Args Types of the format arguments.
 * @param module Module name to prefix the log entry.
 * @param verbosity Verbosity level to use for the log entry.
 * @param file Source file name associated with the log entry.
 * @param line Source line number associated with the log entry.
 * @param format `fmt`-style format string for the message.
 * @param args Arguments matching the format string.
 */
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

