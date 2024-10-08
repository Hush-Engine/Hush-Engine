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

Hush::SharedLibrary::SharedLibrary(void *handle) : m_nativeHandle(handle)
{
}

Hush::SharedLibrary::~SharedLibrary()
{
    if (m_nativeHandle)
    {
#if HUSH_PLATFORM_WIN
        CloseHandle(m_nativeHandle);
#else
#endif
    }
}
Hush::Result<Hush::SharedLibrary, Hush::SharedLibrary::EError> Hush::SharedLibrary::OpenSharedLibrary(
    std::string_view libraryName) noexcept
{
#if HUSH_PLATFORM_WIN
    auto *handle = LoadLibraryA(libraryName.data());
#else
    auto *handle = dlopen(libraryPath.data(), RTLD_LAZY);

#endif

    if (handle == nullptr)
    {
        LogFormat(ELogLevel::Debug, "Failed to open library: {}", libraryName);
        return EError::NotFound;
    }
    return SharedLibrary(handle);
}

void *Hush::SharedLibrary::GetRawSymbol(std::string_view symbolName)
{
#if HUSH_PLATFORM_WIN
    auto *winHandle = static_cast<HMODULE>(m_nativeHandle);

    return GetProcAddress(winHandle, symbolName.data());
#else
    return nullptr;
#endif
}
