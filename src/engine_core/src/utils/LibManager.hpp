//
//  LibManager.hpp
//  embedding_test
//
//  Created by Leonidas Neftali Gonzalez Campos on 13/12/23.
//
#pragma once
#if _WIN32
#include <Shlwapi.h>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#pragma comment(lib, "shlwapi.lib")
#else
#include <dlfcn.h>
#include <unistd.h>
#endif
#if __APPLE__
// Header to get the current exe's path in Mac as well, yup, readlink does not seem to work here
#include <mach-o/dyld.h>
#endif
#include "log/Logger.hpp"
#include <filesystem>
/// <summary>
/// Provides easy access to cross platform library loading functions
/// </summary>
class LibManager
{
  public:
    /// @brief Opens the selected library path (analogous to dlopen)
    /// @param libraryPath Path of the dynamically linked library
    /// @return A handle of the desired library or null
    static void *LibraryOpen(const char *libraryPath);

    /// @brief Loads the desired symbol using a library handle (analogous to dlsym)
    /// @param handle Library handle where the symbol is located
    /// @param symbol Name of the symbol to be loaded
    /// @return A handle to the desired symbol (essentially a function pointer)
    static void *DynamicLoadSymbol(void *handle, const char *symbol);

    /// @brief Gets the parent directory of the current executable file
    /// @return Parent directory path value
    static std::filesystem::path GetCurrentExecutablePath();
};
