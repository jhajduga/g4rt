
// #include "logger.hh"
// #include <filesystem>
// #include <chrono>
// #include <ctime>
// #include <sstream>
// #include <iomanip>
// #undef LOGSVC_DEBUG
// #undef LOGSVC_INFO
// #undef LOGSVC_WARNING
// #undef LOGSVC_ERROR
// #undef LOGSVC_FATAL
// #undef LOG_TO_MODULE


// // Definicje statycznych składowych
// std::recursive_mutex Logger::log_mutex;  // Zmieniono na recursive_mutex
// std::mutex Logger::module_streams_mutex;
// std::unordered_map<std::string, std::string> Logger::module_log_files;
// std::unordered_map<std::string, std::ofstream> Logger::module_log_streams;

// void Logger::Init(int argc, const char* argv[],
//                   const std::string& default_log_file,
//                   loguru::Verbosity verbosity,
//                   int flush_interval_ms)
// {
//     std::cout << "Logger::Init" << std::endl;
//     // Upewnij się, że katalog "logs" istnieje
//     if (!fs::exists("logs")) {
//         fs::create_directory("logs");
//     }

//     loguru::init(argc, const_cast<char**>(argv));

//     // Rotacja globalnego pliku logu, jeśli przekracza limit
//     RotateLogFile(default_log_file, MAX_LOG_SIZE);
//     loguru::g_flush_interval_ms = flush_interval_ms;
//     loguru::add_file(default_log_file.c_str(), loguru::Append, verbosity);
//     loguru::g_stderr_verbosity = verbosity;
// }

#include "logger.hh"
#include <filesystem>
#include <chrono>
#include <ctime>
#undef LOGSVC_DEBUG
#undef LOGSVC_INFO
#undef LOGSVC_WARNING
#undef LOGSVC_ERROR
#undef LOGSVC_FATAL
#undef LOG_TO_MODULE

namespace fs = std::filesystem;

// Definicje statycznych składowych
std::recursive_mutex Logger::log_mutex;
std::mutex Logger::module_streams_mutex;
std::unordered_map<std::string, std::string> Logger::module_log_files;
std::unordered_map<std::string, std::ofstream> Logger::module_log_streams;

// Inicjalizacja start_time – zdefiniuj ją globalnie:
std::chrono::steady_clock::time_point Logger::start_time;

void Logger::Init(int argc, const char* argv[],
                  const std::string& default_log_file,
                  loguru::Verbosity verbosity,
                  int flush_interval_ms)
{
    // Upewnij się, że katalog "logs" istnieje
    if (!fs::exists("logs")) {
        fs::create_directory("logs");
    }
    
    loguru::init(argc, const_cast<char**>(argv));

    // Ustaw moment startu – teraz, po inicjalizacji
    start_time = std::chrono::steady_clock::now();

    loguru::g_flush_interval_ms = flush_interval_ms;
    loguru::add_file(default_log_file.c_str(), loguru::Append, verbosity);
    loguru::g_stderr_verbosity = verbosity;

    // Remove Loguru's default callback so that its preamble (date, time, etc.) isn’t printed.
    loguru::remove_callback(0);

    // Add our maximum custom callback.
    // Here, we're sending output to stderr; you can send it to any FILE*.
    loguru::add_callback("max_custom", 
                     max_custom_log_callback,   // callback function
                     stderr,                    // user_data: pointer to FILE* (e.g. stderr)
                     loguru::Verbosity_MAX,     // set appropriate verbosity level
                     nullptr,                   // no custom close handler
                     nullptr);                  // no custom flush handler
}


void Logger::RotateLogFile(const std::string& filepath, uintmax_t max_size) {
    if (fs::exists(filepath) && fs::file_size(filepath) >= max_size) {
        auto now = std::chrono::system_clock::now();
        std::time_t now_time = std::chrono::system_clock::to_time_t(now);
        std::string new_name = filepath + "_" + std::to_string(now_time);
        fs::rename(filepath, new_name);
    }
}

static std::string format_global_message(loguru::Verbosity verbosity,
                                           const char* file, int line, const char* function,
                                           const std::string& message) {
    std::ostringstream oss;
    
    // Timestamp
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    // Używamy lokalnego czasu; można zmodyfikować format wg potrzeb.
    oss << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S") << " ";
    
    // Uptime – tutaj możesz obliczyć czas od startu aplikacji; dla przykładu wpisujemy "uptime"
    double up = Logger::uptime();
    oss << "Uptime: (" << up << " s)";
    
    // Identyfikator wątku
    oss << "[" << std::this_thread::get_id() << "] ";
    
    // Źródło – plik:linia
    oss << file << ":" << line << " ";
    
    // Poziom logu – prosty przykład
    switch(verbosity) {
        case loguru::Verbosity_MAX: oss << "DEBUG| "; break;
        case loguru::Verbosity_INFO: oss << "INFO | "; break;
        case loguru::Verbosity_WARNING: oss << "WARN | "; break;
        case loguru::Verbosity_ERROR: oss << "ERROR| "; break;
        case loguru::Verbosity_FATAL: oss << "FATAL| "; break;
        default: oss << "     | "; break;
    }
    
    // Wiadomość
    oss << message;
    
    return oss.str();
}

static std::string format_module_message(const std::string& module, loguru::Verbosity verbosity,
                                           const char* file, int line, const char* function,
                                           const std::string& message)
{
    std::ostringstream oss;
    
    // Timestamp
    auto now = std::chrono::system_clock::now();
    std::time_t now_time = std::chrono::system_clock::to_time_t(now);
    // Używamy lokalnego czasu; można zmodyfikować format wg potrzeb.
    oss << std::put_time(std::localtime(&now_time), "%Y-%m-%d %H:%M:%S") << " ";
    
    // Uptime – tutaj możesz obliczyć czas od startu aplikacji; dla przykładu wpisujemy "uptime"
    double up = Logger::uptime();
    oss << "Uptime: (" << up << " s)";
    
    // Identyfikator wątku
    oss << "[" << std::this_thread::get_id() << "] ";
    
    // Źródło – plik:linia
    oss << file << ":" << line << " ";
    
    // Poziom logu – prosty przykład
    switch(verbosity) {
        case loguru::Verbosity_MAX: oss << "DEBUG| "; break;
        case loguru::Verbosity_INFO: oss << "INFO | "; break;
        case loguru::Verbosity_WARNING: oss << "WARN | "; break;
        case loguru::Verbosity_ERROR: oss << "ERROR| "; break;
        case loguru::Verbosity_FATAL: oss << "FATAL| "; break;
        default: oss << "     | "; break;
    }
    
    // Dodaj nazwę modułu
    oss << "[MODULE: " << module << "] ";
    
    // Wiadomość
    oss << message;
    
    return oss.str();
}

namespace fs = std::filesystem;



void Logger::LogDetailed(loguru::Verbosity verbosity, const char* file, int line, const char* function, const std::string& message)
{
    // Tutaj formatujemy komunikat globalny przy użyciu naszej funkcji formattera
    std::string formatted = format_global_message(verbosity, file, line, function, message);
    // Wywołujemy globalne logowanie – komunikat sformatowany będzie widoczny w app.log
    // LOGURU_FMT(s)
    loguru::log(verbosity, file, line, function, formatted);
    loguru::flush();
}


void Logger::AddModuleLog(const std::string& module_name, const std::string& file_path)
{
    std::cout << "Logger::AddModuleLog: module_name = " << module_name 
              << ", file_path = " << file_path << std::endl;
    std::lock_guard<std::recursive_mutex> lock(log_mutex);
    {
        std::lock_guard<std::mutex> lock2(module_streams_mutex);
        std::cout << "Logger::AddModuleLog 2: Sprawdzam istnienie pliku..." << std::endl;
        try {
            if (fs::exists(file_path)) {
                std::cout << "Logger::AddModuleLog: Plik istnieje. Sprawdzam rozmiar..." << std::endl;
                auto size = fs::file_size(file_path);
                std::cout << "Logger::AddModuleLog: Rozmiar pliku: " << size << " bajtów" << std::endl;
                if (size >= MAX_LOG_SIZE) {
                    std::cout << "Logger::AddModuleLog 3: Plik przekracza MAX_LOG_SIZE, rotuję..." << std::endl;
                    RotateLogFile(file_path, MAX_LOG_SIZE);
                }
            } else {
                std::cout << "Logger::AddModuleLog: Plik nie istnieje." << std::endl;
            }
        } catch (const std::exception& ex) {
            std::cerr << "Exception w AddModuleLog: " << ex.what() << std::endl;
        }
        std::cout << "Logger::AddModuleLog 4: Próba otwarcia pliku " << file_path << std::endl;
        module_log_streams[module_name].open(file_path, std::ios::app);
        std::cout << "Logger::AddModuleLog 5: Plik otwarty" << std::endl;
    }
    module_log_files[module_name] = file_path;
}


void Logger::LogToModule(const std::string& module_name, loguru::Verbosity verbosity, const std::string& message,
                           const char* file, int line, const char* function)
{
    // Przygotuj sformatowany komunikat
    std::string formatted = format_module_message(module_name, verbosity, file, line, function, message);
    
    {
        std::lock_guard<std::recursive_mutex> lock(log_mutex);
        if (module_log_streams.find(module_name) == module_log_streams.end()) {
            std::string module_log_path = "logs/" + module_name + ".log";
            AddModuleLog(module_name, module_log_path);
        }
    }
    {
        std::lock_guard<std::mutex> lock(module_streams_mutex);
        module_log_streams[module_name] << formatted << std::endl;
        module_log_streams[module_name].flush();
    }
    std::cout << formatted.c_str() << std::endl;
    loguru::log(verbosity, file, line, function, "%s", formatted.c_str());
    // Globalne logowanie – tutaj możesz przekazać sformatowany komunikat,
    // albo wywołać globalne logowanie i polegać na loguru, np.:
}

void Logger::SetStderrVerbosity(loguru::Verbosity verbosity)
{
    std::lock_guard<std::recursive_mutex> lock(log_mutex);
    loguru::g_stderr_verbosity = verbosity;
}
