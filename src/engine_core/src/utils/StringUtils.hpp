//
//  StringUtils.hpp
//  embedding_test
//
//  Created by Leonidas Neftali Gonzalez Campos on 16/12/23.
//

#pragma once
#include <array>
#include <codecvt>
#include <string>

/// @brief Provides utility functions for handling strings (C and std strings)
class StringUtils
{

  public:
    /// @brief Converts the given char* to a standard wstring, used for Windows, since char_t* is wchar_t*
    /// @param data String to convert
    /// @return wstring of the converted string, from 8B to 16B chars
    static std::wstring ToWString(const char *data);

    static std::string FromWString(std::wstring str);

    template <uint32_t N1, uint32_t N2>
    static constexpr auto CompileTimeConcat(const char (&str1)[N1], const char (&str2)[N2])
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
};
