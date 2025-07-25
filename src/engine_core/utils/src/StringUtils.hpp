//
//  StringUtils.hpp
//  embedding_test
//
//  Created by Leonidas Neftali Gonzalez Campos on 16/12/23.
//

#pragma once
#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <string>
#include <string_view>
/// @brief Provides utility functions for handling strings (C and std strings)
namespace Hush::StringUtils
{

	/// @brief Converts the given char* to a standard wstring, used for Windows, since char_t* is wchar_t*
	/// @param data String to convert
	/// @return wstring of the converted string, from 8B to 16B chars
	std::wstring ToWString(const char *data);

	std::string FromWString(const std::wstring &str);

	inline std::string ToUpper(const std::string_view &str)
	{
		std::string strCpy(str);
		std::transform(strCpy.begin(), strCpy.end(), strCpy.begin(), [](unsigned char c) { return std::toupper(c); });
		return strCpy;
	}

	inline std::string ToLower(const std::string_view &str)
	{
		std::string strCpy(str);
		std::transform(strCpy.begin(), strCpy.end(), strCpy.begin(), [](unsigned char c) { return std::tolower(c); });
		return strCpy;
	}

	constexpr inline std::string_view SubstrView(const std::string &str, int32_t offset, int32_t endIdx)
	{
		return {str.begin() + offset, str.begin() + endIdx};
	}

	template <uint32_t N1, uint32_t N2>
	constexpr auto CompileTimeConcat(const char (&str1)[N1], const char (&str2)[N2])
	{
		std::array<char, N1 + N2 - 1> result{}; // Subtract 1 for the null terminator

		for (uint32_t i = 0; i < N1 - 1; ++i)
		{
			result[i] = str1[i];
		}

		for (uint32_t i = 0; i < N2; ++i)
		{
			result[N1 - 1 + i] = str2[i];
		}

		return result;
	}
}; // namespace Hush::StringUtils
