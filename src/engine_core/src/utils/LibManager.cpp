#include "LibManager.hpp"

constexpr size_t MAX_PATH_LENGTH = 260;

void *LibManager::LibraryOpen(const char *libraryPath)
{
#if _WIN32
    return LoadLibrary(libraryPath);
#else
    return dlopen(libraryPath, RTLD_LAZY);
#endif
}

void *LibManager::DynamicLoadSymbol(void *handle, const char *symbol)
{
#if _WIN32
    auto *handleInstance = static_cast<HINSTANCE>(handle);
    FARPROC processAddress = GetProcAddress(handleInstance, symbol);
    return static_cast<void *>(processAddress);
#else
    return dlsym(handle, symbol);
#endif
}

std::filesystem::path LibManager::GetCurrentExecutablePath()
{
    char buffer[MAX_PATH_LENGTH];
#if defined(_WIN32)
    GetModuleFileName(nullptr, static_cast<char *>(buffer), MAX_PATH_LENGTH);
    // Remove the last bit that contains the executable's name
    PathRemoveFileSpec(static_cast<char *>(buffer));
    return {buffer};
#elif defined(__APPLE__)
    // Use the _NSGetExecutablePath method to get the path
    uint32_t pathLength = MAX_PATH_LENGTH;
    int readPath = _NSGetExecutablePath((char *)buffer, &pathLength);
    if (readPath != 0)
    {
        Hush::LogError("The buffer did not allocate sufficient memory to get the executable's path");
    }
    std::filesystem::path result(buffer);
    return result.parent_path();
#else
    if (readlink("/proc/self/exe", &buffer[0], MAX_PATH_LENGTH) < 0)
    {
        Hush::LogError("The buffer did not allocate sufficient memory to get the executable's path");
    }
    std::filesystem::path result(buffer);
    return result.parent_path();
#endif
}
