#pragma once

#include <loguru.hpp>
#include <fmt/format.h>
#include <string>
#include <unordered_map>
#include <memory>

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
    static void Init(int argc, const char* argv[], const std::string& default_log_file = "logs/app.log",
                     loguru::Verbosity verbosity = loguru::Verbosity_INFO, int flush_interval_ms = 100);

    /**
     * @brief Adds a new module-specific log file.
     * @param module Name of the module.
     * @param log_file Path to the log file for the module.
     * @param verbosity Logging verbosity level.
     */
    static void AddModuleLogFile(const std::string& module, const std::string& log_file, loguru::Verbosity verbosity);

public:
    /**
     * @brief Ustawia poziom logowania do terminala.
     * @param verbosity Poziom logowania (np. loguru::Verbosity_INFO, loguru::Verbosity_ERROR).
     */
    static void SetTerminalLogLevel(loguru::Verbosity verbosity);


    /**
     * @brief Logs a message with debug verbosity for a given module.
     */


template<typename... Args>
/**
 * @brief Logs a debug-level message for the specified module.
 *
 * This function formats a debug message using a printf-style format string and any additional arguments, then logs it with
 * contextual information such as the source file and line number.
 *
 * @param module The module identifier associated with the log entry.
 * @param file The name of the source file from which the log is issued.
 * @param line The line number in the source file where the log is called.
 * @param format A printf-style format string.
 * @param args Additional arguments to be formatted into the log message.
 */
static void LogDebug(const std::string& module, const char* file, int line, const char* format, const Args&... args) {
    logToModule(module, loguru::Verbosity_MAX, file, line, format, args...);
}

template<typename... Args>
/**
 * @brief Logs an informational message for a specific module.
 *
 * This function logs an informational message, utilizing the source file and line number to provide context. It formats the message based on the provided format string and additional arguments, and logs it at the INFO verbosity level.
 *
 * @param module The name of the module associated with the log message.
 * @param file The source file from which the log is issued.
 * @param line The line number in the source file.
 * @param format The format string for the log message.
 * @param args Additional arguments to be formatted into the message.
 */
static void LogInfo(const std::string& module, const char* file, int line, const char* format, const Args&... args) {
    logToModule(module, loguru::Verbosity_INFO, file, line, format, args...);
}

template<typename... Args>
/**
 * @brief Logs a warning message for a specific module.
 *
 * This method formats a log message using the provided format string and additional arguments, and logs it
 * with the warning verbosity level. It also records the location (source file and line number) from where the
 * log is generated.
 *
 * @param module Name of the module related to the log message.
 * @param file Source file name from which the log is invoked.
 * @param line Line number in the source file where the log is generated.
 * @param format Format string used to construct the log message.
 * @param args Additional arguments to format the message.
 */
static void LogWarning(const std::string& module, const char* file, int line, const char* format, const Args&... args) {
    logToModule(module, loguru::Verbosity_WARNING, file, line, format, args...);
}

template<typename... Args>
/**
 * @brief Logs an error message with ERROR severity.
 *
 * Formats and logs an error message for the specified module using the provided source file,
 * line number, format string, and additional arguments. This function delegates the actual
 * logging to the module-specific mechanism with the ERROR verbosity level.
 *
 * @param module Name of the module generating the error.
 * @param file Source file where the log call is made.
 * @param line Line number in the source file corresponding to the log call.
 * @param format Format string for the log message.
 * @param args Additional arguments for formatting the log message.
 */
static void LogError(const std::string& module, const char* file, int line, const char* format, const Args&... args) {
    logToModule(module, loguru::Verbosity_ERROR, file, line, format, args...);
}

template<typename... Args>
/**
 * @brief Logs a fatal error message for the specified module and flushes the log.
 *
 * This function logs a formatted fatal error message using the given module name, source
 * file, line number, and format string with additional arguments, and then immediately 
 * flushes the log output.
 *
 * @param module The module from which the fatal message originates.
 * @param file The name of the source file issuing the log.
 * @param line The line number in the source file issuing the log.
 * @param format The format string for the fatal error message.
 * @param args Additional arguments to be formatted into the message.
 */
static void LogFatal(const std::string& module, const char* file, int line, const char* format, const Args&... args) {
    logToModule(module, loguru::Verbosity_FATAL, file, line, format, args...);
    loguru::flush();
}


private:
    /**
     * @brief Logs a formatted message to the specified module.
     */
template<typename... Args>
/**
 * @brief Formats a log message with module context and logs it.
 *
 * This function formats a message using the provided format string and arguments,
 * prefixes it with the specified module name, and logs the result using the designated
 * verbosity level along with the source file name and line number.
 *
 * @param module Name of the module generating the log message.
 * @param verbosity Logging verbosity level.
 * @param file Source file where the log call originated.
 * @param line Line number in the source file.
 * @param format Format string for the log message.
 * @param args Additional arguments to be formatted into the message.
 */
static void logToModule(const std::string& module, loguru::Verbosity verbosity, const char* file, int line, const char* format, const Args&... args) {
    std::string formatted_message = fmt::format(fmt::runtime(format), args...);
    loguru::log(verbosity, file, line, "[{}] {}", module, formatted_message);
}

static std::unordered_map<std::string, std::shared_ptr<FILE>> module_log_files;
static void moduleLogCallbackImpl(void* user_data, const loguru::Message& message);


};

#define LOGSVC_DEBUG(module, msg, ...)      LogSvc::LogDebug(module, __FILE__, __LINE__, msg, ##__VA_ARGS__)
#define LOGSVC_INFO(module, msg, ...)       LogSvc::LogInfo(module, __FILE__, __LINE__, msg, ##__VA_ARGS__)
#define LOGSVC_WARNING(module, msg, ...)    LogSvc::LogWarning(module, __FILE__, __LINE__, msg, ##__VA_ARGS__)
#define LOGSVC_ERROR(module, msg, ...)      LogSvc::LogError(module, __FILE__, __LINE__, msg, ##__VA_ARGS__)
#define LOGSVC_FATAL(module, msg, ...)      LogSvc::LogFatal(module, __FILE__, __LINE__, msg, ##__VA_ARGS__)
