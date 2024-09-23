/*! \file Logger.cpp
    \author Alan Ramirez Herrera
    \date 2024-03-03
    \brief Logger source implementation
*/

#include "Logger.hpp"

#include <spdlog/spdlog.h>

/// @brief Converts Hush log level to spdlog log level
/// @param level level to convert
/// @return converted log level
static spdlog::level::level_enum HushLogLevelToSpdlog(Hush::ELogLevel level)
{
    switch (level)
    {
    case Hush::ELogLevel::Trace:
        return spdlog::level::trace;
    case Hush::ELogLevel::Debug:
        return spdlog::level::debug;
    case Hush::ELogLevel::Info:
        return spdlog::level::info;
    case Hush::ELogLevel::Warn:
        return spdlog::level::warn;
    case Hush::ELogLevel::Error:
        return spdlog::level::err;
    case Hush::ELogLevel::Critical:
        return spdlog::level::critical;
    }
    return spdlog::level::info;
}

void Hush::Log(Hush::ELogLevel level, std::string_view message)
{
    auto convertedLogLevel = HushLogLevelToSpdlog(level);
    spdlog::log(convertedLogLevel, message);
}