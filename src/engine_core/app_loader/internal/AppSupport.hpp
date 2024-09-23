/*! \file AppSupport.hpp
\author Alan Ramirez
    \date 2024-09-21
    \brief Defines for application loading support.
*/
#pragma once

#include "Platform.hpp"

#if HUSH_PLATFORM_WIN
#define HUSH_SUPPORTS_SHARED_APP 1
#elif HUSH_PLATFORM_LINUX
#define HUSH_SUPPORTS_SHARED_LIB 1
#elif HUSH_PLATFORM_OSX
#define HUSH_SUPPORTS_SHARED_LIB 1
#else
#define HUSH_SUPPORTS_SHARED_LIB 0
#endif

#if HUSH_COMPILER_MSVC
#define HUSH_WEAK
#elif HUSH_COMPILER_GCC || HUSH_COMPILER_CLANG
#define HUSH_WEAK __attribute__((weak))
#endif