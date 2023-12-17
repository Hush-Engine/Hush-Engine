#include "LibManager.hpp"

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
