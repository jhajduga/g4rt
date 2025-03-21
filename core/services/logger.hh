#ifndef LOGGER_HH
#define LOGGER_HH
// #define LOGURU_USE_FMTLIB 1
// #include "loguru.hpp"
#include <loguru.hpp>
#include <string>
#include <iostream>
#include <mutex>
#include <unordered_map>
#include <fstream>
#include <fmt/format.h>
#include <thread>      // dla std::this_thread::get_id()
#include <functional>  // dla std::hash
#include <vector>      // dla std::vector
#include <cstdio>
#include <ctime>


class Logger {
public:

/**
 * @brief Formats and outputs a log message with a custom timestamp and verbosity level.
 *
 * This callback generates the current local timestamp, retrieves the verbosity level from the
 * log message (falling back on the numeric value if necessary), and writes the formatted output
 * to the file stream provided via user_data.
 *
 * @param user_data Pointer to a FILE stream used for outputting the log message.
 * @param message  Loguru message containing the log text and metadata.
 */
static void max_custom_log_callback(void* user_data, const loguru::Message& message) {
    FILE* out = static_cast<FILE*>(user_data);

    // Get the current local time.
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    char time_buffer[32];
    std::strftime(time_buffer, sizeof(time_buffer), "%Y-%m-%d %H:%M:%S", std::localtime(&now_time));

    // Use Loguru's get_verbosity_name() to convert the verbosity enum to a string.
    // (If get_verbosity_name() returns nullptr, fall back to printing the numeric value.)
    const char* verbosity_name = loguru::get_verbosity_name(message.verbosity);
    if (!verbosity_name) {
        static char numeric[16];
        snprintf(numeric, sizeof(numeric), "%d", message.verbosity);
        verbosity_name = numeric;
    }

    // Here we choose to output only a custom timestamp, the verbosity, and the log message.
    // You can remove or add any parts as you see fit.
    fprintf(out, "[%s] [%s] %s\n", time_buffer, verbosity_name, message.message);
}

//
// Pomocnicza funkcja do rotacji pliku logu.
// Jeśli plik istnieje i jego rozmiar >= max_size, przemianowuje plik (dodając znacznik czasu).
//
static void RotateLogFile(const std::string& filepath, uintmax_t max_size);
    /**
     * Inicjalizuje logger.
     * @param argc Liczba argumentów (z main)
     * @param argv Argumenty (z main)
     * @param default_log_file Domyślny plik logu (np. "logs/app.log")
     * @param verbosity Poziom logowania (np. loguru::Verbosity_INFO)
     * @param flush_interval_ms Jeśli > 0, loguru w tle flushuje co zadany interwał (w ms)
     */
    static void Init(int argc, const char* argv[],
                     const std::string& default_log_file = "logs/app.log",
                     loguru::Verbosity verbosity = loguru::Verbosity_INFO,
                     int flush_interval_ms = 100);

    /**
     * @brief Computes the elapsed time in seconds since the logger was initialized.
     *
     * Calculates the uptime by determining the difference between the current steady clock time and the logger's
     * start time, converting the result from milliseconds to seconds.
     *
     * @return double The uptime in seconds.
     */
    static double uptime() {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);
        return duration.count() / 1000.0;
    }

    template<typename... Args>
    /**
     * @brief Logs a formatted debug message with source context.
     *
     * This templated function formats a debug message using a printf-style format string and
     * the provided arguments, then logs the resulting message at the debug verbosity level.
     * The log entry automatically includes the source file name, line number, and function name.
     *
     * @param format The printf-style format string.
     * @param args Arguments to be formatted into the message.
     */
    static void LogDebug(const char* format, const Args&... args) {
        loguru::log(loguru::Verbosity_MAX, __FILE__, __LINE__, __FUNCTION__,
                    fmt::format(fmt::runtime(format), args...).c_str());
    }

    template<typename... Args>
    /**
     * @brief Logs an informational message.
     *
     * Formats the provided message using a runtime format string and additional arguments,
     * then logs it at the INFO verbosity level. The log entry automatically includes the source
     * file, line, and function name for debugging purposes.
     *
     * @param format A dynamic format string following fmt::format syntax.
     * @param args Values to be formatted into the message.
     */
    static void LogInfo(const char* format, const Args&... args) {
        loguru::log(loguru::Verbosity_INFO, __FILE__, __LINE__, __FUNCTION__,
                    fmt::format(fmt::runtime(format), args...).c_str());
    }

    template<typename... Args>
    /**
     * @brief Logs a formatted warning message.
     *
     * This function formats a message using the provided format string and arguments, and logs it with warning-level severity.
     * The log entry automatically includes the source file, line number, and function name.
     *
     * @param format The fmt-compatible format string for the log message.
     * @param args Additional arguments to format the message.
     */
    static void LogWarning(const char* format, const Args&... args) {
        loguru::log(loguru::Verbosity_WARNING, __FILE__, __LINE__, __FUNCTION__,
                    fmt::format(fmt::runtime(format), args...).c_str());
    }

    template<typename... Args>
    /**
     * @brief Logs an error message.
     *
     * Formats a message using the given format string and arguments, then logs it as an error. The log entry includes source file, line number, and function name for context.
     *
     * @param format The format string for the error message.
     * @param args Arguments to be formatted into the message.
     */
    static void LogError(const char* format, const Args&... args) {
        loguru::log(loguru::Verbosity_ERROR, __FILE__, __LINE__, __FUNCTION__,
                    fmt::format(fmt::runtime(format), args...).c_str());
    }

    template<typename... Args>
    /**
     * @brief Logs a fatal error message and immediately flushes the log.
     *
     * This function formats a log message using a printf-style format string and corresponding arguments,
     * then logs it with fatal verbosity. It automatically attaches the source file, line number, and function
     * name where the call is made. The log is flushed immediately after the message is logged to ensure that
     * critical errors are recorded promptly.
     *
     * @param format A C-style format string specifying the fatal error message.
     * @param args Variadic arguments that are formatted into the message.
     */
    static void LogFatal(const char* format, const Args&... args) {
        loguru::log(loguru::Verbosity_FATAL, __FILE__, __LINE__, __FUNCTION__,
                    fmt::format(fmt::runtime(format), args...).c_str());
        loguru::flush(); // natychmiastowy flush dla komunikatów krytycznych
    }

    /**
     * LogDetailed – loguje wiadomość, przyjmując sformatowany ciąg znaków oraz dodatkowe informacje o miejscu wywołania.
     */
    static void LogDetailed(loguru::Verbosity verbosity, const char* file, int line, const char* function, const std::string& message);

    /**
     * Rejestruje moduł – otwiera plik logu specyficzny dla modułu.
     */
    static void AddModuleLog(const std::string& module_name, const std::string& file_path);

    /**
     * Loguje do modułu – zapisuje komunikat bezpośrednio do pliku modułowego
     * (oraz wywołuje globalne logowanie, jeśli chcesz, aby komunikat trafił także do globalnego logu).
     * Wiadomość powinna mieć już dodany prefiks (np. przez makro LOG_TO_MODULE).
     */
    static void LogToModule(const std::string& module_name, loguru::Verbosity verbosity, const std::string& message,
                            const char* file, int line, const char* function);

    /**
     * Szablonowa wersja LogToModule – formatuje wiadomość i automatycznie przekazuje __FILE__, __LINE__, __FUNCTION__.
     */
    template<typename... Args>
    /**
     * @brief Logs a formatted message to a specified module.
     *
     * This templated method formats the provided message using the fmt library and logs it to the module-specific log file. It automatically captures and records the source file, line number, and function name for precise debugging context.
     *
     * @param module_name Name of the module for which the log is intended.
     * @param verbosity Log level indicating the severity of the message.
     * @param format A printf-style format string.
     * @param args Arguments to be formatted into the message.
     */
    static void LogToModule(const std::string& module_name, loguru::Verbosity verbosity, const char* format, const Args&... args) {
        LogToModule(module_name, verbosity, fmt::format(fmt::runtime(format), args...), __FILE__, __LINE__, __FUNCTION__);
    }

    /**
     * Ustawia poziom logowania na stderr (konsoli).
     */
    static void SetStderrVerbosity(loguru::Verbosity verbosity);

private:
    static std::chrono::steady_clock::time_point start_time;
    static std::recursive_mutex log_mutex;  // Zmieniono na recursive_mutex
    static std::unordered_map<std::string, std::string> module_log_files;
    static std::unordered_map<std::string, std::ofstream> module_log_streams;
    static std::mutex module_streams_mutex; // mutex dla operacji na module_log_streams

    // Maksymalny rozmiar pliku logu przed rotacją (np. 5 MB)
    static const uintmax_t MAX_LOG_SIZE = 5 * 1024 * 1024;
};

// Makra globalne – standardowe wywołania
#define LOGSVC_DEBUG(msg, ...)      Logger::LogDetailed(loguru::Verbosity_MAX, __FILE__, __LINE__, __FUNCTION__, fmt::format(fmt::runtime(msg), ##__VA_ARGS__))
#define LOGSVC_INFO(msg, ...)       Logger::LogDetailed(loguru::Verbosity_INFO, __FILE__, __LINE__, __FUNCTION__, fmt::format(fmt::runtime(msg), ##__VA_ARGS__))
#define LOGSVC_WARNING(msg, ...)    Logger::LogDetailed(loguru::Verbosity_WARNING, __FILE__, __LINE__, __FUNCTION__, fmt::format(fmt::runtime(msg), ##__VA_ARGS__))
#define LOGSVC_ERROR(msg, ...)      Logger::LogDetailed(loguru::Verbosity_ERROR, __FILE__, __LINE__, __FUNCTION__, fmt::format(fmt::runtime(msg), ##__VA_ARGS__))
#define LOGSVC_FATAL(msg, ...)      Logger::LogDetailed(loguru::Verbosity_FATAL, __FILE__, __LINE__, __FUNCTION__, fmt::format(fmt::runtime(msg), ##__VA_ARGS__))

#define LOG_TO_MODULE(module, verbosity, msg, ...) \
    Logger::LogToModule(module, verbosity, fmt::format(fmt::runtime("[MODULE: {}] " msg), module, ##__VA_ARGS__), __FILE__, __LINE__, __FUNCTION__)

// Makra rozszerzone – dodają dodatkowe informacje, np. identyfikator wątku.
#define LOGSVC_INFO_EXT(msg, ...) \
    Logger::LogDetailed(loguru::Verbosity_INFO, __FILE__, __LINE__, __FUNCTION__, \
        fmt::format(fmt::runtime("[Thread: {}] " msg), std::this_thread::get_id(), ##__VA_ARGS__))

#define LOGSVC_DEBUG_EXT(msg, ...) \
    Logger::LogDetailed(loguru::Verbosity_MAX, __FILE__, __LINE__, __FUNCTION__, \
        fmt::format(fmt::runtime("[Thread: {}] " msg), std::this_thread::get_id(), ##__VA_ARGS__))

// Specjalizacja formattera dla std::thread::id – umożliwia formatowanie identyfikatora wątku.
namespace fmt {
    template <>
    struct formatter<std::thread::id> {
        /**
 * @brief Parses the format specification for a custom formatter.
 *
 * This function performs minimal parsing by returning the iterator to the beginning of the
 * provided format parse context, indicating that no custom format specifiers are processed.
 *
 * @param ctx The context that contains the format string to be parsed.
 * @return An iterator pointing to the beginning of the format string.
 */
constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
        template <typename FormatContext>
        /**
         * @brief Formats a thread identifier using its hash representation.
         *
         * Computes the hash value of the provided thread identifier and writes the result to
         * the output iterator from the given formatting context. This hash-based representation
         * offers a concise and consistent view of thread identifiers.
         *
         * @param id The thread identifier to format.
         * @param ctx The context providing the output iterator for formatting.
         * @return An iterator pointing to the end of the formatted output.
         */
        auto format(const std::thread::id& id, FormatContext& ctx) {
            auto hash = std::hash<std::thread::id>{}(id);
            return format_to(ctx.out(), "{}", hash);
        }
    };

    // Specjalizacja dla std::vector<T> – umożliwia formatowanie wektorów.
    template<typename T>
    struct formatter<std::vector<T>> {
        /**
 * @brief Parses a format specifier for a custom formatter.
 *
 * This function satisfies the interface required by the formatting library for custom types.
 * It performs no additional parsing and simply returns an iterator to the beginning of the format string,
 * indicating that no custom format specifiers are supported.
 *
 * @param ctx The format parse context containing the format string.
 * @return An iterator pointing to the beginning of the format string.
 */
constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
        template<typename FormatContext>
        /**
         * @brief Formats a vector as a comma-separated list enclosed in square brackets.
         *
         * This function converts each element of the vector to a string using fmt::to_string(),
         * concatenating the results with a comma and a space, and enclosing the entire list in square brackets.
         * The formatted string is then written to the provided format context.
         *
         * @tparam T The type of the elements in the vector.
         * @param vec The vector whose elements will be formatted.
         * @param ctx The format context used to output the resulting string.
         * @return An iterator pointing to the end of the formatted output in the context.
         */
        auto format(const std::vector<T>& vec, FormatContext& ctx) {
            std::string result = "[";
            bool first = true;
            for (const auto& item : vec) {
                if (!first)
                    result += ", ";
                else
                    first = false;
                result += fmt::to_string(item);
            }
            result += "]";
            return format_to(ctx.out(), "{}", result);
        }
    };
}

#endif // LOGGER_HH
