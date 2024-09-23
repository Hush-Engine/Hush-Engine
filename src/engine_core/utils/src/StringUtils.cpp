#include "StringUtils.hpp"
#include "Logger.hpp"

#if _WIN32

#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

std::wstring StringUtils::ToWString(const char *data)
{
    int bytesToAlloc = MultiByteToWideChar(CP_UTF8, 0, data, -1, nullptr, 0);
    if (bytesToAlloc <= 0)
    {
        Hush::LogError("Failed to convert UTF8 string to wide string!");
        return {};
    }
    auto buffer = std::make_unique<wchar_t[]>(bytesToAlloc);
    MultiByteToWideChar(CP_UTF8, 0, data, -1, buffer.get(), bytesToAlloc);
    return {buffer.get()};
}
std::string StringUtils::FromWString(std::wstring str)
{
    int bytesToAlloc =
        WideCharToMultiByte(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), nullptr, 0, nullptr, nullptr);
    if (bytesToAlloc <= 0)
    {
        Hush::LogError("Failed to convert wide string to UTF8!");
        return {};
    }
    auto buffer = std::make_unique<char[]>(bytesToAlloc);
    WideCharToMultiByte(CP_UTF8, 0, str.c_str(), static_cast<int>(str.size()), buffer.get(), bytesToAlloc, nullptr,
                        nullptr);

    return {buffer.get()};
}
#endif
