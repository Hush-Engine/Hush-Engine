#include "StringUtils.hpp"
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include "Logger.hpp"

#if _WIN32
using UTF8ToUTF16Convert = std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>>;
std::wstring StringUtils::ToWString(const char *data)
{
	int bytesToAlloc = MultiByteToWideChar(CP_UTF8, 0, data, -1, nullptr, 0);
	if (bytesToAlloc <= 0)
	{
		LOG_ERROR_LN("Failed to convert UTF8 string to wide string!");
        return {};
	}
    wchar_t *buffer = new wchar_t[bytesToAlloc];
    MultiByteToWideChar(CP_UTF8, 0, data, -1, buffer, bytesToAlloc);

	return { buffer };
}
std::string StringUtils::FromWString(std::wstring str)
{
	char *buffer = nullptr;
	int bytesToAlloc = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), str.size(), buffer, 0, nullptr, nullptr);
	if (bytesToAlloc <= 0)
	{
		LOG_ERROR_LN("Failed to convert wide string to UTF8!");
		return {};
	}
	buffer = new char[bytesToAlloc];
	WideCharToMultiByte(CP_UTF8, 0, str.c_str(), str.size(), buffer, bytesToAlloc, nullptr, nullptr);

	return { buffer };
}
#endif
