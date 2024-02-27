//
//  StringUtils.hpp
//  embedding_test
//
//  Created by Leonidas Neftali Gonzalez Campos on 16/12/23.
//

#pragma once
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
};