//
//  LibManager.hpp
//  embedding_test
//
//  Created by Leonidas Neftali Gonzalez Campos on 13/12/23.
//
#pragma once
#if WIN32
#include <windows.h>
#include <Shlwapi.h>
#pragma comment(lib, "shlwapi.lib")
#else
#include <dlfcn.h>
#include <unistd.h>
#endif
#if __APPLE__
//Header to get the current exe's path in Mac as well, yup, readlink does not seem to work here
#include <mach-o/dyld.h>
#endif
#include <filesystem>
#include "../../../Core/Logger.hpp"
/// <summary>
/// Provides easy access to cross platform library loading functions
/// </summary>
class LibManager {
public:
	static void* LibraryOpen(const char* libraryPath);

	static void* DynamicLoadSymbol(void* handle, const char* symbol);

	static std::filesystem::path GetCurrentExecutablePath();


};
