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

// A maximum-custom callback with full control over output formatting.
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

    static double uptime() {
        auto now = std::chrono::steady_clock::now();
        auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - start_time);
        return duration.count() / 1000.0;
    }

    template<typename... Args>
    static void LogDebug(const char* format, const Args&... args) {
        loguru::log(loguru::Verbosity_MAX, __FILE__, __LINE__, __FUNCTION__,
                    fmt::format(fmt::runtime(format), args...).c_str());
    }

    template<typename... Args>
    static void LogInfo(const char* format, const Args&... args) {
        loguru::log(loguru::Verbosity_INFO, __FILE__, __LINE__, __FUNCTION__,
                    fmt::format(fmt::runtime(format), args...).c_str());
    }

    template<typename... Args>
    static void LogWarning(const char* format, const Args&... args) {
        loguru::log(loguru::Verbosity_WARNING, __FILE__, __LINE__, __FUNCTION__,
                    fmt::format(fmt::runtime(format), args...).c_str());
    }

    template<typename... Args>
    static void LogError(const char* format, const Args&... args) {
        loguru::log(loguru::Verbosity_ERROR, __FILE__, __LINE__, __FUNCTION__,
                    fmt::format(fmt::runtime(format), args...).c_str());
    }

    template<typename... Args>
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
        constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
        template <typename FormatContext>
        auto format(const std::thread::id& id, FormatContext& ctx) {
            auto hash = std::hash<std::thread::id>{}(id);
            return format_to(ctx.out(), "{}", hash);
        }
    };

    // Specjalizacja dla std::vector<T> – umożliwia formatowanie wektorów.
    template<typename T>
    struct formatter<std::vector<T>> {
        constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
        template<typename FormatContext>
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
