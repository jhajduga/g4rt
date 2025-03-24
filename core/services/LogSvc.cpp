// LogSvc.cpp
#include "LogSvc.hpp"
#include <filesystem>

std::unordered_map<std::string, std::shared_ptr<FILE>> LogSvc::module_log_files;

void LogSvc::Init(int argc, const char* argv[], const std::string& default_log_file,
                  loguru::Verbosity verbosity, int flush_interval_ms) {
    char** nonConstArgv = const_cast<char**>(argv);
    loguru::init(argc, nonConstArgv);

    SetVerbosity(verbosity);

#ifdef PROJECT_LOCATION_PATH
    SetLogFolder(PROJECT_LOCATION_PATH + std::string("/logs"));
#else
    SetLogFolder("logs");
#endif

    std::string full_path = s_log_folder + "/" + default_log_file;
    s_main_log_id = full_path;
    loguru::add_file(full_path.c_str(), loguru::Append, s_verbosity);
    loguru::g_flush_interval_ms = flush_interval_ms;
}

void LogSvc::SetTerminalLogLevel(loguru::Verbosity verbosity) {
    loguru::g_stderr_verbosity = verbosity;
}

void LogSvc::ReconfigureMainLog(const std::string& new_log_full_path) {
    if (!s_main_log_id.empty()) {
        loguru::remove_callback(s_main_log_id.c_str());
    }

    // Extract directory from full path and update log folder
    std::filesystem::path full_path(new_log_full_path);
    s_log_folder = full_path.parent_path().string();

    // Save the full path as ID
    s_main_log_id = new_log_full_path;
    loguru::add_file(new_log_full_path.c_str(), loguru::Append, s_verbosity);
}

void LogSvc::AddModuleLogFile(const std::string& module, const std::string& full_log_path, loguru::Verbosity verbosity) {
    FILE* file = fopen(full_log_path.c_str(), "a");
    if (!file) {
        throw std::runtime_error("Failed to open log file for module: " + module);
    }

    {
        std::lock_guard<std::mutex> guard(module_log_files_mutex);
        module_log_files[module] = std::shared_ptr<FILE>(file, [](FILE* f) { fclose(f); });
    }

    loguru::add_callback(
        module.c_str(),
        [](void* user_data, const loguru::Message& message) {
            const std::string* target_module = static_cast<const std::string*>(user_data);
            std::string prefix = "[" + *target_module + "]";

            std::string message_text(message.message);

            if (message_text.find(prefix) == 0) {
                std::lock_guard<std::mutex> guard(module_log_files_mutex);
                auto it = LogSvc::module_log_files.find(*target_module);
                if (it != LogSvc::module_log_files.end()) {
                    FILE* file = it->second.get();
                    if (file) {
                        fprintf(file, "%s%s%s\n", message.preamble, message.indentation, message.message);
                        fflush(file);
                    }
                }
            }
        },
        new std::string(module),
        verbosity,
        [](void* user_data) { delete static_cast<std::string*>(user_data); }
    );
    
    
}



