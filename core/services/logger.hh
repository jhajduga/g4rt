#ifndef LOGGER_HH
#define LOGGER_HH

#include <loguru.hpp>
#include <string>
#include <iostream>
#include <mutex>
#include <unordered_map>
#include <fmt/format.h>
#include <thread>      // dla std::this_thread::get_id()
#include <functional>  // dla std::hash
#include <vector>      // dla std::vector

class Logger {
public:
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

    // Funkcje logujące – wersje szablonowe z fmtlib

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
     * Rejestruje moduł – jeśli moduł o podanej nazwie nie został jeszcze dodany, tworzy osobny plik logu.
     */
    static void AddModuleLog(const std::string& module_name, const std::string& file_path);

    /**
     * Loguje do modułu – wersja podstawowa, przyjmuje także informacje o miejscu wywołania.
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
     * Ustawia poziom logowania na stderr (konsoli). Może być wywołane w czasie działania aplikacji.
     */
    static void SetStderrVerbosity(loguru::Verbosity verbosity);

private:
    static std::mutex log_mutex;
    static std::unordered_map<std::string, std::string> module_log_files;

    // Maksymalny rozmiar pliku logu przed rotacją (tutaj 5 MB)
    static const uintmax_t MAX_LOG_SIZE = 5 * 1024 * 1024;
};

// Makra ułatwiające korzystanie z loggera
#define LOGSVC_DEBUG(msg, ...) Logger::LogDetailed(loguru::Verbosity_MAX, __FILE__, __LINE__, __FUNCTION__, fmt::format(fmt::runtime(msg), ##__VA_ARGS__))
#define LOGSVC_INFO(msg, ...)  Logger::LogDetailed(loguru::Verbosity_INFO, __FILE__, __LINE__, __FUNCTION__, fmt::format(fmt::runtime(msg), ##__VA_ARGS__))
#define LOGSVC_WARNING(msg, ...)  Logger::LogDetailed(loguru::Verbosity_WARNING, __FILE__, __LINE__, __FUNCTION__, fmt::format(fmt::runtime(msg), ##__VA_ARGS__))
#define LOGSVC_ERROR(msg, ...)  Logger::LogDetailed(loguru::Verbosity_ERROR, __FILE__, __LINE__, __FUNCTION__, fmt::format(fmt::runtime(msg), ##__VA_ARGS__))
#define LOGSVC_FATAL(msg, ...)  Logger::LogDetailed(loguru::Verbosity_FATAL, __FILE__, __LINE__, __FUNCTION__, fmt::format(fmt::runtime(msg), ##__VA_ARGS__))

// Makra rozszerzone – dodają dodatkowe informacje (np. identyfikator wątku)
#define LOGSVC_INFO_EXT(msg, ...) \
    Logger::LogDetailed(loguru::Verbosity_INFO, __FILE__, __LINE__, __FUNCTION__, \
        fmt::format(fmt::runtime("[Thread: {}] " msg), std::this_thread::get_id(), ##__VA_ARGS__))

#define LOGSVC_DEBUG_EXT(msg, ...) \
    Logger::LogDetailed(loguru::Verbosity_MAX, __FILE__, __LINE__, __FUNCTION__, \
        fmt::format(fmt::runtime("[Thread: {}] " msg), std::this_thread::get_id(), ##__VA_ARGS__))

// Specjalizacja formattera dla std::thread::id, aby umożliwić logowanie identyfikatora wątku.
namespace fmt {
    template <>
    struct formatter<std::thread::id> {
        // parse – nie obsługujemy żadnych dodatkowych opcji, więc zwracamy początek ciągu
        constexpr auto parse(format_parse_context& ctx) { return ctx.begin(); }
        template <typename FormatContext>
        auto format(const std::thread::id& id, FormatContext& ctx) {
            // Używamy hash, aby uzyskać reprezentację identyfikatora.
            auto hash = std::hash<std::thread::id>{}(id);
            return format_to(ctx.out(), "{}", hash);
        }
    };

    // Specjalizacja dla std::vector<T> – umożliwia formatowanie wektorów
    template<typename T>
    struct formatter<std::vector<T>> {
        // Ignorujemy opcje formatowania
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
