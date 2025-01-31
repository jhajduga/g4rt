#ifndef LOGGER_HH
#define LOGGER_HH

#include <loguru.hpp>
#include <string>
#include <iostream>
#include <mutex>
#include <unordered_map>

class Logger {
public:
    // Initialize the logger
    static void Init(int argc, const char* argv[], const std::string& default_log_file = "logs/app.log", loguru::Verbosity verbosity = loguru::Verbosity_INFO);

    // Log message with different verbosity levels
    // static void LogTrace(const std::string& message);
    static void LogDebug(const std::string& message);
    static void LogInfo(const std::string& message);
    static void LogWarning(const std::string& message);
    static void LogError(const std::string& message);
    static void LogFatal(const std::string& message);

    // Log with file, line, and function information
    static void LogDetailed(loguru::Verbosity verbosity, const char* file, int line, const char* function, const std::string& message);

    // Log to a specific module with its own file
    static void AddModuleLog(const std::string& module_name, const std::string& file_path);
    static void LogToModule(const std::string& module_name, loguru::Verbosity verbosity, const std::string& message);

private:
    static std::mutex log_mutex;
    static std::unordered_map<std::string, std::string> module_log_files;
};

// #define LOG_TRACE_F(msg, ...) Logger::LogDetailed(loguru::Verbosity_MAX, __FILE__, __LINE__, __FUNCTION__, fmt::format(msg, ##__VA_ARGS__))
#define LOG_DEBUG_F(msg, ...) Logger::LogDetailed(loguru::Verbosity_MAX, __FILE__, __LINE__, __FUNCTION__, fmt::format(msg, ##__VA_ARGS__))
#define LOG_INFO_F(msg, ...) Logger::LogDetailed(loguru::Verbosity_INFO, __FILE__, __LINE__, __FUNCTION__, fmt::format(msg, ##__VA_ARGS__))
#define LOG_WARNING_F(msg, ...) Logger::LogDetailed(loguru::Verbosity_WARNING, __FILE__, __LINE__, __FUNCTION__, fmt::format(msg, ##__VA_ARGS__))
#define LOG_ERROR_F(msg, ...) Logger::LogDetailed(loguru::Verbosity_ERROR, __FILE__, __LINE__, __FUNCTION__, fmt::format(msg, ##__VA_ARGS__))
#define LOG_FATAL_F(msg, ...) Logger::LogDetailed(loguru::Verbosity_FATAL, __FILE__, __LINE__, __FUNCTION__, fmt::format(msg, ##__VA_ARGS__))

#endif // LOGGER_HH