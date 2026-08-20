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

Hush::SharedLibrary::SharedLibrary(void *handle)
	: m_nativeHandle(handle)
{
}

Hush::SharedLibrary::SharedLibrary(SharedLibrary &&rhs) noexcept
	: m_nativeHandle(std::exchange(rhs.m_nativeHandle, nullptr))
{
}

Hush::SharedLibrary::~SharedLibrary()
{
	if (m_nativeHandle != nullptr)
	{
#if HUSH_PLATFORM_WIN
		FreeLibrary(static_cast<HMODULE>(m_nativeHandle));
#else
		dlclose(m_nativeHandle);
#endif
	}
}

Hush::Result<Hush::SharedLibrary, Hush::SharedLibrary::EError> Hush::SharedLibrary::OpenSharedLibrary(
	NullTerminatedStringView libraryName) noexcept
{
#if HUSH_PLATFORM_WIN
	auto *handle = LoadLibraryA(libraryName.c_str());
#else
	auto *handle = dlopen(libraryName.c_str(), RTLD_LAZY);

#endif

	if (handle == nullptr)
	{
		LogFormat(ELogLevel::Debug, "Failed to open library: {}", std::string_view(libraryName));
		return EError::NotFound;
	}
	return SharedLibrary(handle);
}

void *Hush::SharedLibrary::GetRawSymbol(NullTerminatedStringView symbolName)
{
#if HUSH_PLATFORM_WIN
	auto *winHandle = static_cast<HMODULE>(m_nativeHandle);

	return reinterpret_cast<void *>(GetProcAddress(winHandle, symbolName.c_str()));
#else
	return nullptr;
#endif
}
