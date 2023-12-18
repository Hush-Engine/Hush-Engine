#include "LibManager.hpp"

#define MAX_PATH_LENGTH 260


void* LibManager::LibraryOpen(const char* libraryPath)
{
#if WIN32
    return LoadLibrary(libraryPath);
#else
    return dlopen(libraryPath, RTLD_LAZY);
#endif
}

void* LibManager::DynamicLoadSymbol(void* handle, const char* symbol)
{
#if WIN32
    HINSTANCE handleInstance = (HINSTANCE)handle;
    FARPROC processAddress = GetProcAddress(handleInstance, symbol);
    return (void*)(intptr_t)processAddress;
#else
    return dlsym(handle, symbol);
#endif
}

std::filesystem::path LibManager::GetCurrentExecutablePath()
{
    char buffer[MAX_PATH_LENGTH];
#if WIN32
    GetModuleFileName(nullptr, buffer, MAX_PATH_LENGTH);
    //Remove the last bit that contains the executable's name
    PathRemoveFileSpec(buffer);
#else
    readlink("/proc/self/", buffer, MAX_PATH_LENGTH);
#endif
    return std::filesystem::path(buffer);
}

