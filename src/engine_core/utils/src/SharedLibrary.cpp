/*! \file SharedLibrary.hpp
	\author Alan Ramirez
	\date 2024-09-22
	\brief Shared Library implementation
*/

#include "SharedLibrary.hpp"
#include "Logger.hpp"
#include "Platform.hpp"

#include <utility>

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

Hush::SharedLibrary &Hush::SharedLibrary::operator=(SharedLibrary &&rhs) noexcept
{
	if (this != &rhs)
	{
		std::swap(m_nativeHandle, rhs.m_nativeHandle);
	}
	return *this;
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
	return OpenSharedLibrary(std::filesystem::path(std::string(std::string_view(libraryName))));
}

Hush::Result<Hush::SharedLibrary, Hush::SharedLibrary::EError> Hush::SharedLibrary::OpenSharedLibrary(
	const std::filesystem::path &libraryPath) noexcept
{
	if (libraryPath.empty())
	{
		return EError::EmptyName;
	}
#if HUSH_PLATFORM_WIN
	auto *handle = LoadLibraryW(libraryPath.c_str());
#else
	const std::string nativePath = libraryPath.string();
	auto *handle = dlopen(nativePath.c_str(), RTLD_LAZY);
#endif

	if (handle == nullptr)
	{
		LogFormat(ELogLevel::Debug, "Failed to open library: {}", libraryPath.generic_string());
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
	return dlsym(m_nativeHandle, symbolName.c_str());
#endif
}
