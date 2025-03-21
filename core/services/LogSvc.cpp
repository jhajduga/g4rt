#include "LogSvc.hpp"
#include <loguru.hpp>
#include <fmt/format.h>

// Definicja mapy dla plików modułów
std::unordered_map<std::string, std::shared_ptr<FILE>> LogSvc::module_log_files;

void LogSvc::Init(int argc, const char* argv[], const std::string& default_log_file, loguru::Verbosity verbosity, int flush_interval_ms) {
    char** nonConstArgv = const_cast<char**>(argv);
    loguru::init(argc, nonConstArgv);
    std::string project_path = PROJECT_LOCATION_PATH;
    std::string full_log_path = project_path + "/" + default_log_file;
    loguru::add_file(full_log_path.c_str(), loguru::Append, verbosity);
    loguru::g_flush_interval_ms = flush_interval_ms;
    
}

void LogSvc::SetTerminalLogLevel(loguru::Verbosity verbosity) {
    loguru::g_stderr_verbosity = verbosity;
}


void LogSvc::AddModuleLogFile(const std::string& module, const std::string& log_file, loguru::Verbosity verbosity) {
    std::string project_path = PROJECT_LOCATION_PATH;
    std::string full_log_path = project_path + "/" + log_file;
    FILE* file = fopen(full_log_path.c_str(), "a");
    if (!file) {
        throw std::runtime_error("Failed to open log file for module: " + module);
    }

    // Zapis pliku do mapy modułów
    module_log_files[module] = std::shared_ptr<FILE>(file, [](FILE* f) { fclose(f); });

    // Rejestracja callbacka z filtrem wiadomości na podstawie modułu
    loguru::add_callback(
        module.c_str(),
        [](void* user_data, const loguru::Message& message) {
            const std::string* target_module = static_cast<const std::string*>(user_data);
            std::string prefix = "[" + *target_module + "]";

            // Konwersja message.message na std::string
            std::string message_text(message.message);

            // Sprawdzenie, czy wiadomość zaczyna się od prefiksu modułu
            if (message_text.find(prefix) == 0) {
                FILE* file = LogSvc::module_log_files[*target_module].get();
                if (file) {
                    fprintf(file, "%s%s%s\n", message.preamble, message.indentation, message.message);
                    fflush(file);
                }
            }
        },
        new std::string(module),
        verbosity,
        [](void* user_data) { delete static_cast<std::string*>(user_data); }
    );
}

// Implementacja callbacka logowania
void LogSvc::moduleLogCallbackImpl(void* user_data, const loguru::Message& message) {
    FILE* file = static_cast<FILE*>(user_data);
    if (file) {
        fprintf(file, "%s%s%s\n", message.preamble, message.indentation, message.message);
        fflush(file);
    }
}
