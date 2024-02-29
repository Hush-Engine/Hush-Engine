/*! \file ThreadingUtils.hpp
	\author Kyn21kx
	\date 2024-02-28
	\brief Provides platform-independent threading utility functions, DOES NOT PROVIDE A THREAD CLASS
*/

#pragma once
#include <cstdint>

#if WIN32
#include <windows.h>
#else
#include <unistd.h>
#endif

class ThreadingUtils
{
public:
	static void SleepForMilliseconds(uint32_t milliseconds) {
#if WIN32
        Sleep(milliseconds);
#else
        sleep(milliseconds / 1000);
#endif
	}
};
