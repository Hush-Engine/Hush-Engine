/*! \file Platform.hpp
    \author Alan Ramirez Herrera
    \date 2024-03-03
    \brief Platform detection macros
*/

#pragma once

#ifdef _WIN64
#define HUSH_PLATFORM_WIN 1
#define HUSH_PLATFORM_LINUX 0
#define HUSH_PLATFORM_OSX 0

#elif defined(__linux__) && !defined(__ANDROID__)
#define HUSH_PLATFORM_LINUX 1
#define HUSH_PLATFORM_WIN 0
#define HUSH_PLATFORM_OSX 0
#elif defined(__APPLE__) && defined(__MACH__)
#define HUSH_PLATFORM_OSX 1
#define HUSH_PLATFORM_LINUX 0
#define HUSH_PLATFORM_WIN 0
#else
#error "Platform not supported"
#endif