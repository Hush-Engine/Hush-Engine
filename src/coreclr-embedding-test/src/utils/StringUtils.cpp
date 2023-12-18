#include "StringUtils.hpp"

typedef std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> UTF8ToUTF16Convert;

std::wstring StringUtils::ToWString(const char* data)
{
	UTF8ToUTF16Convert converter;
	std::wstring convertedStr = converter.from_bytes(data);
	return convertedStr;
}
