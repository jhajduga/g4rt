/**
  *
  * \author P.Matejski
  * \date 2023-04-07
  *   
**/

#ifndef Dose3D_LOGSVC_H
#define Dose3D_LOGSVC_H

#define SPDLOG_ACTIVE_LEVEL trace

#include <map>
#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/logger.h>
#include <fmt/format.h>
#include <G4ThreeVector.hh>
#include "LogSvcConfig.hh"

/// \brief Initialize local logger, class name as logger name
#define LOGSVC_LOGGER_INITIALIZE m_logger = LogSvc::GetLogger(typeid(*this).name());

extern std::shared_ptr<spdlog::logger> m_logger;

/// \brief Log message to local logger
#define LOGSVC_TRACE(...) if (m_logger != nullptr) {SPDLOG_LOGGER_TRACE(m_logger,__VA_ARGS__);} else {SPDLOG_TRACE(__VA_ARGS__);}
/// \brief Log message to local logger
#define LOGSVC_DEBUG(...) if (m_logger != nullptr) {SPDLOG_LOGGER_DEBUG(m_logger,__VA_ARGS__);} else {SPDLOG_DEBUG(__VA_ARGS__);}
/// \brief Log message to local logger
#define LOGSVC_INFO(...) if (m_logger != nullptr) {SPDLOG_LOGGER_INFO(m_logger,__VA_ARGS__);} else {SPDLOG_INFO(__VA_ARGS__);}
/// \brief Log message to local logger
#define LOGSVC_WARN(...) if (m_logger != nullptr) {SPDLOG_LOGGER_WARN(m_logger,__VA_ARGS__);} else {SPDLOG_WARN(__VA_ARGS__);}
/// \brief Log message to local logger
#define LOGSVC_ERROR(...) if (m_logger != nullptr) {SPDLOG_LOGGER_ERROR(m_logger,__VA_ARGS__);} else {SPDLOG_ERROR(__VA_ARGS__);}
/// \brief Log message to local logger
#define LOGSVC_CRITICAL(...) if (m_logger != nullptr) {SPDLOG_LOGGER_CRITICAL(m_logger,__VA_ARGS__);} else {SPDLOG_CRITICAL(__VA_ARGS__);}



/// \brief Log message to default logger
#define LOG_THIS_TRACE(...) SPDLOG_TRACE(__VA_ARGS__);
/// \brief Log message to default logger
#define LOG_THIS_DEBUG(...) SPDLOG_DEBUG(__VA_ARGS__);
/// \brief Log message to default logger
#define LOG_THIS_INFO(...) SPDLOG_INFO(__VA_ARGS__);
/// \brief Log message to default logger
#define LOG_THIS_WARN(...) SPDLOG_WARN(__VA_ARGS__);
/// \brief Log message to default logger
#define LOG_THIS_ERROR(...) SPDLOG_ERROR(__VA_ARGS__);
/// \brief Log message to default logger
#define LOG_THIS_CRITICAL(...) SPDLOG_CRITICAL(__VA_ARGS__);



///\class LogSvc
///\brief The application log service
class LogSvc  {
    public:
    ///\brief Static method to get instance of this singleton object.
      static LogSvc *GetInstance();

    /// @brief Initialize log service for use with default logger
    static void Initialize();
    static bool Initialized();
    /// @brief Configure initialized service with config file
    static void Configure();
    /// @brief Set global log level
    /// @param logLevelStr Log level
    static void DefaulLogLevel(std::string logLevelStr);
    /// @brief  Get logger refernce and create if not exists
    /// @param loggerName Logger name
    /// @return Logger reference
    static std::shared_ptr<spdlog::logger> GetLogger(std::string loggerName);
    static std::shared_ptr<spdlog::sinks::basic_file_sink_mt> GetFileSink(std::string fileName);

    static void SetLoggerLogLevel(std::shared_ptr<spdlog::logger> loggertPtr, std::string newLogLevel);
    
    static void SetConsolePattern(std::string newPattern);
    static void SetFilePattern(std::string newPattern);

    static std::shared_ptr<spdlog::logger> RecreateLogger(std::string loggerName);
    static void UpdateLoggers();
    static void ShutDown();

    private:
    LogSvc();
    // Delete the copy and move constructors
    LogSvc(const LogSvc &) = delete;
    LogSvc &operator=(const LogSvc &) = delete;
    LogSvc(LogSvc &&) = delete;
    LogSvc &operator=(LogSvc &&) = delete;

    static std::map<std::string,std::shared_ptr<spdlog::logger>> m_loggersMap;
    static std::shared_ptr<spdlog::sinks::stdout_color_sink_mt> m_consoleSink;
    static std::shared_ptr<spdlog::logger> m_defaultLogger;
    static std::map<std::string,std::shared_ptr<spdlog::sinks::basic_file_sink_mt>> m_fileSinks;
    static std::map<std::string,std::shared_ptr<spdlog::sinks::basic_file_sink_mt>> m_loggerFileSinks;
    ///
    static std::shared_ptr<LogSvcConfig> m_config;


    /// @brief Configure new logger with stored parameters
    /// @param logger Logger name
    static void ConfigureLogger(std::shared_ptr<spdlog::logger> logger);
    /// @brief Convert log level name to enum
    /// @param logLevelStr Log level name
    /// @return Log level as enum
    static spdlog::level::level_enum Str2LogLevel(std::string logLevelStr);
};

template<>
struct fmt::formatter<G4ThreeVector> : fmt::formatter<std::string>
{
    auto format(G4ThreeVector my, format_context &ctx)  const -> decltype(ctx.out())
    {
        return format_to(ctx.out(), "{},{},{}", my.getX(), my.getY(), my.getZ());
    }
};
#endif //Dose3D_LOGSVC_H