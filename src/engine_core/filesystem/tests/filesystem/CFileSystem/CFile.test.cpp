/*! \file CFile.test.cpp
	\author Alan Ramirez
	\date 2024-12-24
	\brief CFile tests
*/
#include "filesystem/CFileSystem/CFile.hpp"

#include <Logger.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem/helper.hpp>
#include <fstream>
#include <random>

static Hush::CFile OpenFile(const std::string &path, const Hush::EFileOpenMode mode)
{
	FILE *file = nullptr;
	#if HUSH_PLATFORM_WIN
	if (mode == Hush::EFileOpenMode::Read)
	{
		fopen_s(&file, path.c_str(), "r");
	}
	else
	{
		fopen_s(&file, path.c_str(), "w");
	}
	#else
	if (mode == Hush::EFileOpenMode::Read)
    {
        file = fopen(path.c_str(), "r");
    }
    else
    {
        file = fopen(path.c_str(), "w");
    }
	#endif

	// Get file size
	fseek(file, 0, SEEK_END);
	const std::size_t fileSize = ftell(file);
	fseek(file, 0, SEEK_SET);

	Hush::FileInfo metadata;

	metadata.mode = mode;
	metadata.path = path;
	metadata.size = fileSize;
	metadata.lastModified = 0;

	return {file, std::move(metadata)};
}

TEST_CASE("CFile tests")
{
	using namespace Hush::Tests;
	// Create a temporary file
	const std::string tempFileContent = Hush::Tests::GenerateRandomString(1024);

	SECTION("Write file")
	{
		// Arrange
		const std::string tempFilePath = "temp_file_write.txt";
		auto file = OpenFile(tempFilePath, Hush::EFileOpenMode::Write);

		// Act
		// Write the file
		// Create a span from the string
		std::span data(reinterpret_cast<const std::byte *>(tempFileContent.data()), tempFileContent.size());
		const auto writeResult = file.Write(data);

		// Assert
		REQUIRE(writeResult.has_value());

		// Clean up
		file.Close();
		DeleteFile(tempFilePath);
	}

	SECTION("File metadata has correct values")
	{
		// Arrange
		const std::string tempFilePath = "temp_file_metadata.txt";
		CreateTempFile(tempFilePath, tempFileContent);

		// Act
		auto file = OpenFile(tempFilePath, Hush::EFileOpenMode::Read);
		const auto &metadata = file.GetFileInfo();

		// Assert
		REQUIRE(metadata.path == tempFilePath);
		REQUIRE(metadata.size == tempFileContent.size());
		REQUIRE(metadata.lastModified == 0);
		REQUIRE(metadata.mode == Hush::EFileOpenMode::Read);

		file.Close();
		DeleteFile(tempFilePath);
	}

	SECTION("Read file")
	{
		// Arrange
		const std::string tempFilePath = "temp_file_read.txt";
		CreateTempFile(tempFilePath, tempFileContent);

		// Act
		auto file = OpenFile(tempFilePath, Hush::EFileOpenMode::Read);

		// Read the file
		std::vector<std::byte> data(file.GetFileInfo().size);
		const auto readResult = file.Read(data);

		// Assert
		REQUIRE(readResult.has_value());
		REQUIRE(
			std::ranges::equal(tempFileContent, data, [](char c, std::byte b) { return c == static_cast<char>(b); }));

		file.Close();
		DeleteFile(tempFilePath);
	}
}
