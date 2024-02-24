/*! \file BindingLogger.cpp
    \author Alan Ramirez Herrera
    \date 2024-02-24
    \brief This file contains a wrapper over the C++ logger to provide bindings for other languages that do not support
   variadic templates
*/

#include "core/log/Logger.hpp"

#include <spdlog/sinks/stdout_color_sinks.h>

void LogMessage(LogLevel level, const char *message)
{
    Log(level, message);
}
