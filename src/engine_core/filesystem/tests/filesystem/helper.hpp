/*! \file helper.hpp
    \author Alan Ramirez
    \date 2024-12-24
    \brief Helper functions
*/

#pragma once
#include <fstream>
#include <random>
#include <string>

namespace Hush::Tests
{
    inline void CreateTempFile(const std::string &path, const std::string &content)
    {
        std::ofstream file(path);
        file << content;
        file.close();
    }

    inline std::string GenerateRandomString(const std::size_t length)
    {
        std::string str;
        str.reserve(length);

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution dis(static_cast<std::int32_t>('a'), static_cast<std::int32_t>('z'));

        std::generate_n(std::back_inserter(str), length, [&]() { return dis(gen); });

        return str;
    }

    inline void DeleteFile(const std::string &path)
    {
        std::filesystem::remove(path);
    }
}; // namespace Hush::Tests