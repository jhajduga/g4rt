#include "logger.hh"

std::mutex Logger::log_mutex;
std::unordered_map<std::string, std::string> Logger::module_log_files;

void Logger::Init(int argc, const char* argv[], const std::string& default_log_file, loguru::Verbosity verbosity) {
    loguru::init(argc, const_cast<char**>(argv));
    loguru::add_file(default_log_file.c_str(), loguru::Append, verbosity);
    loguru::g_stderr_verbosity = verbosity;
    // loguru::flush_every(1);  // Flush co każde logowanie
}

// void Logger::LogTrace(const std::string& message) {
//     std::lock_guard<std::mutex> lock(log_mutex);
//     LOG_F(, "%s", message.c_str());
// }

void Logger::LogDebug(const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex);
    LOG_F(MAX, "%s", message.c_str());
    loguru::flush();
}

void Logger::LogInfo(const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex);
    LOG_F(INFO, "%s", message.c_str());
    loguru::flush();
}

void Logger::LogWarning(const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex);
    LOG_F(WARNING, "%s", message.c_str());
    loguru::flush();
}

void Logger::LogError(const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex);
    LOG_F(ERROR, "%s", message.c_str());
    loguru::flush();
}

void Logger::LogFatal(const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex);
    LOG_F(FATAL, "%s", message.c_str());
    loguru::flush();
}

void Logger::LogDetailed(loguru::Verbosity verbosity, const char* file, int line, const char* function, const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex);
    loguru::log(verbosity, file, line, function, message.c_str());
    loguru::flush();
}

void Logger::AddModuleLog(const std::string& module_name, const std::string& file_path) {
    std::lock_guard<std::mutex> lock(log_mutex);
    if (module_log_files.find(module_name) == module_log_files.end()) {
        module_log_files[module_name] = file_path;
        loguru::add_file(file_path.c_str(), loguru::Append, loguru::Verbosity_INFO);
    }
}

void Logger::LogToModule(const std::string& module_name, loguru::Verbosity verbosity, const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex);
    if (module_log_files.find(module_name) != module_log_files.end()) {
        loguru::log(verbosity, __FILE__, __LINE__, __FUNCTION__, message.c_str());
    } else {
        LOG_F(WARNING, "Module '%s' log file not found. Logging to default.", module_name.c_str());
        loguru::log(verbosity, __FILE__, __LINE__, __FUNCTION__, message.c_str());
    }
}