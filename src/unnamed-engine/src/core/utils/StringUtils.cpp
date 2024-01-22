#include "core/utils/StringUtils.hpp"

#if WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
std::wstring StringUtils::ToWString(const char *data)
{
    int size = MultiByteToWideChar(CP_UTF8, 0, data, -1, nullptr, 0);
    std::wstring convertedStr(size, 0);
    MultiByteToWideChar(CP_UTF8, 0, data, -1, &convertedStr[0], size);
    return convertedStr;
}

std::string StringUtils::FromWString(std::wstring str)
{
    int size = WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, nullptr, 0, nullptr, nullptr);
    std::string convertedStr(size, 0);
    WideCharToMultiByte(CP_UTF8, 0, str.c_str(), -1, &convertedStr[0], size, nullptr, nullptr);
    return convertedStr;
}
#endif
