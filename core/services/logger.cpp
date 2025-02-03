#include "logger.hh"
#include <filesystem>
#include <chrono>
#include <ctime>

// Używamy przestrzeni nazw dla std::filesystem (C++17)
namespace fs = std::filesystem;

// Definicje statycznych składowych
std::mutex Logger::log_mutex;
std::unordered_map<std::string, std::string> Logger::module_log_files;

// Pomocnicza funkcja do rotacji pliku logu
// Jeśli plik istnieje i jego rozmiar >= max_size, plik jest przemianowywany (dodajemy znacznik czasu)
static void RotateLogFile(const std::string& filepath, uintmax_t max_size) {
    if (fs::exists(filepath) && fs::file_size(filepath) >= max_size) {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        // Tworzymy nową nazwę pliku, np.: "logs/app.log" -> "logs/app_<timestamp>.log"
        std::string new_name = filepath + "_" + std::to_string(now_time);
        fs::rename(filepath, new_name);
    }
}

void Logger::Init(int argc, const char* argv[],
                  const std::string& default_log_file,
                  loguru::Verbosity verbosity,
                  int flush_interval_ms)
{
    loguru::init(argc, const_cast<char**>(argv));

    // Przed dodaniem pliku wykonujemy rotację, jeśli plik już istnieje i jest duży
    RotateLogFile(default_log_file, MAX_LOG_SIZE);

    loguru::g_flush_interval_ms = flush_interval_ms; // Flushowanie w tle (jeśli flush_interval_ms > 0)
    loguru::add_file(default_log_file.c_str(), loguru::Append, verbosity);
    loguru::g_stderr_verbosity = verbosity;
}

void Logger::LogDetailed(loguru::Verbosity verbosity, const char* file, int line, const char* function, const std::string& message)
{
    loguru::log(verbosity, file, line, function, "%s", message.c_str());
}

void Logger::AddModuleLog(const std::string& module_name, const std::string& file_path)
{
    std::lock_guard<std::mutex> lock(log_mutex);
    if (module_log_files.find(module_name) == module_log_files.end()) {
        // Przed dodaniem pliku modułowego wykonujemy rotację, jeśli plik jest zbyt duży
        RotateLogFile(file_path, MAX_LOG_SIZE);
        module_log_files[module_name] = file_path;
        loguru::add_file(file_path.c_str(), loguru::Append, loguru::Verbosity_INFO);
    }
}

void Logger::LogToModule(const std::string& module_name, loguru::Verbosity verbosity, const std::string& message,
                           const char* file, int line, const char* function)
{
    std::lock_guard<std::mutex> lock(log_mutex);
    // Jeśli dany moduł nie został jeszcze dodany, rejestrujemy go – domyślnie plik "logs/<module_name>.log"
    if (module_log_files.find(module_name) == module_log_files.end()) {
        std::string module_log_path = "logs/" + module_name + ".log";
        // Rotacja pliku modułowego, jeśli potrzeba
        RotateLogFile(module_log_path, MAX_LOG_SIZE);
        module_log_files[module_name] = module_log_path;
        loguru::add_file(module_log_path.c_str(), loguru::Append, verbosity);
    }
    loguru::log(verbosity, file, line, function, "%s", message.c_str());
}

void Logger::SetStderrVerbosity(loguru::Verbosity verbosity)
{
    std::lock_guard<std::mutex> lock(log_mutex);
    loguru::g_stderr_verbosity = verbosity;
}
