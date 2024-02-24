//
//  ScriptingManager.hpp
//  embedding_test
//
//  Created by Leonidas Neftali Gonzalez Campos on 25/12/23.
//

#pragma once

#include <spdlog/spdlog.h>

enum class LogLevel : std::uint32_t
{
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    Critical
};

namespace internal_
{
/// @brief Convertion enum from spdlog level_enum to LogLevel
constexpr std::array SPDLOG_TO_LOG_LEVEL_CONVERTION = {
    LogLevel::Trace, LogLevel::Debug, LogLevel::Info, LogLevel::Warning, LogLevel::Error, LogLevel::Critical,
};

/// @brief Convertion array from LogLevel to spdlog
constexpr std::array LOG_LEVEL_TO_SPDLOG_CONVERTION = {
    spdlog::level::level_enum::trace, spdlog::level::level_enum::debug, spdlog::level::level_enum::info,
    spdlog::level::level_enum::warn,  spdlog::level::level_enum::err,   spdlog::level::level_enum::critical};

/// @brief convert spdlog level to LogLevel
/// @param level spdlog level
/// @return LogLevel
inline LogLevel spdlogToLogLevel(spdlog::level::level_enum level)
{
    return SPDLOG_TO_LOG_LEVEL_CONVERTION[static_cast<size_t>(level)];
}

/// @brief convert LogLevel to spdlog level
/// @param level LogLevel
/// @return spdlog level
inline spdlog::level::level_enum logLevelToSpdlog(LogLevel level)
{
    return LOG_LEVEL_TO_SPDLOG_CONVERTION[static_cast<size_t>(level)];
}
} // namespace internal_

/// @brief Logs a message with a given leve
/// @tparam ...Args arguments to log
/// @param level level of the log
/// @param ...args arguments to log
template <typename... Args> void Log(LogLevel level, Args &&...args)
{
    using spdlog::level::level_enum;
    const level_enum spdlogLevel = internal_::logLevelToSpdlog(level);
    spdlog::default_logger()->log(spdlogLevel, std::forward<Args>(args)...);
}

/// @brief Logs a info message
/// @tparam ...Args arguments to log
/// @param ...args arguments to log
template <typename... Args> void LogInfo(Args &&...args)
{
    Log(LogLevel::Info, std::forward<Args>(args)...);
}

/// @brief Logs a warning message
/// @tparam ...Args arguments to log
/// @param ...args arguments to log
template <typename... Args> void LogWarn(Args &&...args)
{
    Log(LogLevel::Warning, std::forward<Args>(args)...);
}

/// @brief Logs a error message
/// @tparam ...Args arguments to log
/// @param ...args arguments to log
template <typename... Args> void LogError(Args &&...args)
{
    Log(LogLevel::Error, std::forward<Args>(args)...);
}

/// @brief Logs a critical message
/// @tparam ...Args arguments to log
/// @param ...args arguments to log
template <typename... Args> void LogCritical(Args &&...args)
{
    Log(LogLevel::Critical, std::forward<Args>(args)...);
}

/// @brief Logs a debug message
/// @tparam ...Args arguments to log
/// @param ...args arguments to log
template <typename... Args> void LogDebug(Args &&...args)
{
    Log(LogLevel::Debug, std::forward<Args>(args)...);
}

/// @brief Logs a trace message
/// @tparam ...Args arguments to log
/// @param ...args arguments to log
template <typename... Args> void LogTrace(Args &&...args)
{
    Log(LogLevel::Trace, std::forward<Args>(args)...);
}

/// @brief Logs a message with a given level
/// @param level level of the log
/// @param message message of the log, is const char* as this makes it easier to use in C#
void LogMessage(LogLevel level, const char *message);

void ConfigureLogs();
