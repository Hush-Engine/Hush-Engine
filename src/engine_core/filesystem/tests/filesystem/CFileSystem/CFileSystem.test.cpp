/*! \file CFileSystem.test.cpp
    \author Alan Ramirez
    \date 2024-12-24
    \brief CFileSystem tests
*/
#include "filesystem/CFileSystem/CFileSystem.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>

#include "filesystem/helper.hpp"

TEST_CASE("CFileSystem tests")
{
    using namespace Hush::Tests;

    // Create a temporary file
    auto filesystem = Hush::CFileSystem(".");

    SECTION("Open file")
    {
        // Arrange
        const std::string tempFilePath = "cfile_system_test_open.txt";

        // Act
        auto file = filesystem.OpenFile(".", tempFilePath, Hush::EFileOpenMode::Write);

        // Assert
        REQUIRE(file.has_value());

        // Clean up
        auto &fileRef = file.assume_value();
        fileRef->Close();
        DeleteFile(tempFilePath);
    }
}