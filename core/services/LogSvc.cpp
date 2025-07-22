// LogSvc.cpp
#include "LogSvc.hh"
#include "LogSession.hh"
#include <filesystem>

std::unordered_map<std::string, std::shared_ptr<FILE>> LogSvc::module_log_files;

void LogSvc::Init(int argc, const char* argv[], const std::string& default_log_file,
                  loguru::Verbosity verbosity, int flush_interval_ms) {
    loguru::init(argc, const_cast<char**>(argv));
    loguru::g_stderr_verbosity = loguru::Verbosity_OFF;
    loguru::add_stack_cleanup("std::__cxx11::basic_string<char, std::char_traits<char>, std::allocator<char> >", "std::string");
    static LogSession logSession;



#ifdef PROJECT_LOCATION_PATH
    SetLogFolder(PROJECT_LOCATION_PATH + std::string("/logs"));
#else
    SetLogFolder("logs");
#endif
    std::string full_path = s_log_folder + "/" + default_log_file;
    s_main_log_id = full_path;
    loguru::add_file(full_path.c_str(), loguru::Append, s_terminal_verbosity);
    loguru::g_flush_interval_ms = flush_interval_ms;
    EnableCustomVerbosityNames();
    EnableColoredTerminalOutput();
}

void LogSvc::SetTerminalLogLevel(loguru::Verbosity verbosity) {
    s_terminal_verbosity = verbosity;
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
    loguru::add_file(new_log_full_path.c_str(), loguru::Append, s_terminal_verbosity);
}

void LogSvc::SetThreadName(const std::string& name) {
    loguru::set_thread_name(name.c_str());
}

void LogSvc::AddModuleLogFile(const std::string& module, const std::string& full_log_path, loguru::Verbosity verbosity) {
    FILE* file = fopen(full_log_path.c_str(), "a");
    if (!file) {
        throw std::runtime_error("Failed to open log file for module: " + module);
    }

    std::lock_guard<std::mutex> guard(module_log_files_mutex);
    module_log_files[module] = std::shared_ptr<FILE>(file, [](FILE* f) { fclose(f); });

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

void LogSvc::EnableColoredTerminalOutput() {
    loguru::add_callback(
        "colored_terminal",
        [](void*, const loguru::Message& msg) {
            if (msg.verbosity > s_terminal_verbosity) return;

            const char* color = "";
            switch (msg.verbosity) {
                case loguru::Verbosity_FATAL:   color = loguru::terminal_red();        break;
                case loguru::Verbosity_ERROR:   color = loguru::terminal_light_red();  break;
                case loguru::Verbosity_WARNING: color = loguru::terminal_yellow();     break;
                case loguru::Verbosity_INFO:    color = loguru::terminal_green();      break;
                case 9:                         color = loguru::terminal_purple();     break;
                default:                        color = "";      break;
            }

            fprintf(stderr, "%s%s%s%s\n",
                    color,
                    msg.preamble,
                    msg.message,
                    loguru::terminal_reset());

            if (loguru::g_flush_interval_ms == 0) {
                fflush(stderr);
            }
        },
        nullptr,
        loguru::Verbosity_MAX
    );
}

void LogSvc::EnableCustomVerbosityNames() {
    loguru::set_verbosity_to_name_callback([](loguru::Verbosity v) -> const char* {
        switch (v) {
            case loguru::Verbosity_FATAL:   return "FATAL";
            case loguru::Verbosity_ERROR:   return "ERROR";
            case loguru::Verbosity_WARNING: return "WARNING";
            case loguru::Verbosity_INFO:    return "INFO";
            case 9:                         return "DEBUG";
            default:                        return nullptr;
        }
    });

    loguru::set_name_to_verbosity_callback([](const char* name) -> loguru::Verbosity {
        if (strcmp(name, "FATAL") == 0) return loguru::Verbosity_FATAL;
        if (strcmp(name, "ERROR") == 0) return loguru::Verbosity_ERROR;
        if (strcmp(name, "WARNING")  == 0) return loguru::Verbosity_WARNING;
        if (strcmp(name, "INFO")  == 0) return loguru::Verbosity_INFO;
        if (strcmp(name, "DEBUG") == 0) return 9;
        return loguru::Verbosity_INVALID;
    });
}

loguru::Verbosity LogSvc::ParseVerbosityLevel(const std::string& level_str) {
    std::string upper;
    upper.reserve(level_str.size());
    for (char c : level_str) {
        upper += std::toupper(static_cast<unsigned char>(c));
    }

    if (upper == "DEBUG") {
        return loguru::Verbosity_MAX;
    }

    loguru::Verbosity level = loguru::get_verbosity_from_name(upper.c_str());

    if (level == loguru::Verbosity_INVALID) {
        char* end = nullptr;
        int lvl = std::strtol(upper.c_str(), &end, 10);
        if (end && *end == '\0' && lvl >= loguru::Verbosity_FATAL && lvl <= loguru::Verbosity_MAX) {
            return static_cast<loguru::Verbosity>(lvl);
        }
    }

    return level;
}
