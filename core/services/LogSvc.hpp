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
static void LogDebug(const std::string& module, const char* file, int line, const char* format, const Args&... args) {
    logToModule(module, loguru::Verbosity_MAX, file, line, format, args...);
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
    /**
     * @brief Logs a formatted message to the specified module.
     */
template<typename... Args>
static void logToModule(const std::string& module, loguru::Verbosity verbosity, const char* file, int line, const char* format, const Args&... args) {
    std::string formatted_message = fmt::format(format, args...);
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
