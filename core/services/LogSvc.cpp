#include "LogSvc.hpp"
#include <loguru.hpp>
#include <fmt/format.h>

// Definicja mapy dla plików modułów
std::unordered_map<std::string, std::shared_ptr<FILE>> LogSvc::module_log_files;

/**
 * @brief Initializes the loguru logging system.
 *
 * Configures loguru by processing the provided command-line arguments, adding
 * a default log file with specified verbosity, and setting the flush interval
 * for log output.
 *
 * @param argc Number of command-line arguments.
 * @param argv Array of command-line argument strings.
 * @param default_log_file Path to the default log file for appending log messages.
 * @param verbosity Verbosity level for the log file output.
 * @param flush_interval_ms Time interval, in milliseconds, for flushing logs.
 */
void LogSvc::Init(int argc, const char* argv[], const std::string& default_log_file, loguru::Verbosity verbosity, int flush_interval_ms) {
    char** nonConstArgv = const_cast<char**>(argv);
    loguru::init(argc, nonConstArgv);
    loguru::add_file(default_log_file.c_str(), loguru::Append, verbosity);
    loguru::g_flush_interval_ms = flush_interval_ms;
    
}

/**
 * @brief Sets the global logging verbosity for terminal output.
 *
 * Configures the logging system to use the specified verbosity level for messages written to standard error (stderr).
 *
 * @param verbosity The desired logging verbosity level.
 */
void LogSvc::SetTerminalLogLevel(loguru::Verbosity verbosity) {
    loguru::g_stderr_verbosity = verbosity;
}


/**
 * @brief Adds a log file for a specified module.
 *
 * Opens the specified log file in append mode and registers a callback that logs messages
 * for the module if they start with the module’s prefix (formatted as "[module]"). Matching
 * messages are written to the log file with their preamble and indentation, and the output
 * is flushed immediately.
 *
 * If the log file cannot be opened, a std::runtime_error is thrown.
 *
 * @param module The name of the module used as a prefix filter for log messages.
 * @param log_file The file path where the module's log messages will be appended.
 * @param verbosity The minimum verbosity level at which the callback will process log messages.
 */
void LogSvc::AddModuleLogFile(const std::string& module, const std::string& log_file, loguru::Verbosity verbosity) {
    FILE* file = fopen(log_file.c_str(), "a");
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

/**
 * @brief Writes a formatted log message to a module-specific log file.
 *
 * This callback extracts a FILE pointer from the provided user data, concatenates the
 * log message's preamble, indentation, and main text, then writes the result to the file.
 * It flushes the file to ensure that the log entry is promptly recorded.
 *
 * @param user_data Pointer to the FILE where the log message will be written.
 * @param message   Structure containing the log message components: preamble, indentation, and message.
 */
void LogSvc::moduleLogCallbackImpl(void* user_data, const loguru::Message& message) {
    FILE* file = static_cast<FILE*>(user_data);
    if (file) {
        fprintf(file, "%s%s%s\n", message.preamble, message.indentation, message.message);
        fflush(file);
    }
}
