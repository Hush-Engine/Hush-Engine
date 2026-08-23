#include "LibManager.hpp"

#include <string_view>
#include <vector>

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

void LibManager::LibraryClose(void *handle)
{
	if (handle == nullptr)
	{
		return;
	}
#if _WIN32
	FreeLibrary(static_cast<HMODULE>(handle));
#else
	dlclose(handle);
#endif
}

std::filesystem::path LibManager::GetCurrentExecutablePath()
{
#if defined(_WIN32)
	std::vector<wchar_t> buffer(MAX_PATH_LENGTH);
	while (true)
	{
		const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
		if (length == 0)
		{
			Hush::LogError("Could not get the executable path");
			return {};
		}
		if (length < buffer.size())
		{
			return std::filesystem::path(std::wstring_view(buffer.data(), length)).parent_path();
		}
		buffer.resize(buffer.size() * 2);
	}
#elif defined(__APPLE__)
	uint32_t pathLength = 0;
	_NSGetExecutablePath(nullptr, &pathLength);
	std::vector<char> buffer(pathLength);
	if (_NSGetExecutablePath(buffer.data(), &pathLength) != 0)
	{
		Hush::LogError("Could not get the executable path");
		return {};
	}
	std::filesystem::path result(buffer.data());
	return result.parent_path();
#else
	std::vector<char> buffer(MAX_PATH_LENGTH);
	while (true)
	{
		const ssize_t length = readlink("/proc/self/exe", buffer.data(), buffer.size());
		if (length < 0)
		{
			Hush::LogError("Could not get the executable path");
			return {};
		}
		if (static_cast<std::size_t>(length) < buffer.size())
		{
			return std::filesystem::path(std::string_view(buffer.data(), static_cast<std::size_t>(length))).parent_path();
		}
		buffer.resize(buffer.size() * 2);
	}
#endif
}
