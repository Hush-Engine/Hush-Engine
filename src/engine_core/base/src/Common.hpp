/*! \file Common.hpp
	\author Alan Ramirez
	\date 2024-10-13
	\brief Common definitions
*/

#pragma once

#include "Platform.hpp"

// By default export the libs.
#ifndef HUSH_EXPORT_LIB
#define HUSH_EXPORT_LIB 1
#endif

#if HUSH_PLATFORM_WIN

#if HUSH_EXPORT_LIB
#define HUSH_EXPORT __declspec(dllexport)
#else
#define HUSH_IMPORT __declspec(dllimport)
#endif

#else
#define HUSH_EXPORT
#endif