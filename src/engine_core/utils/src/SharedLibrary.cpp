/*! \file SharedLibrary.hpp
\author Alan Ramirez
    \date 2024-09-22
    \brief Shared Library implementation
*/

#include "SharedLibrary.hpp"
#include "Platform.hpp"

#include <LibManager.hpp>

#if HUSH_PLATFORM_WIN
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#else
#include <dlfcn.h>
#include <unistd.h>
#endif

SharedLibrary::SharedLibrary(void *handle) : m_nativeHandle(handle)
{
}

SharedLibrary SharedLibrary::OpenSharedLibrary(std::string_view libraryName)
{
#if HUSH_PLATFORM_WIN
    return SharedLibrary{LoadLibraryA(libraryName.data())};
#else
    return dlopen(libraryPath, RTLD_LAZY);
#endif
}
void *SharedLibrary::GetRawSymbol(std::string_view symbolName)
{
#if HUSH_PLATFORM_WIN
    auto *winHandle = static_cast<HMODULE>(m_nativeHandle);

    return GetProcAddress(winHandle, symbolName.data());
#else
    return nullptr;
#endif
}
