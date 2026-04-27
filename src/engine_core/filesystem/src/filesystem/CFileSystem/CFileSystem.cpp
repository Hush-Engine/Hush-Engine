/*! \file FsResourceSource.cpp
	\author Alan Ramirez
	\date 2024-11-17
	\brief Resource Source interface that uses the filesystem (C API)
*/
#include "CFileSystem.hpp"

#include "CFile.hpp"
#include "Assertions.hpp"
#include "IFile.hpp"
#include "StringUtils.hpp"

#include <Logger.hpp>
#include <array>
#include <cstdint>
#include <filesystem>
#include <system_error>
#include <cerrno>
#include <string_view>

#if HUSH_PLATFORM_EMSCRIPTEN
#include <sys/stat.h>
#endif

Hush::CFileSystem::CFileSystem(std::string_view root)
	: m_root(root)
{
}

Hush::Result<std::unique_ptr<Hush::IFile>, Hush::IFile::EError> Hush::CFileSystem::OpenFile(
	std::filesystem::path vfsPath, std::filesystem::path path, EFileOpenMode mode)
{
	std::array<char, 4> modeStr{};

	if (mode == EFileOpenMode::Read)
	{
		modeStr = {'r', 'b', '\0'};
	}
	else if (mode == EFileOpenMode::Write)
	{
		modeStr = {'w', 'b', '\0'};
	}
	else if (mode == EFileOpenMode::ReadWrite)
	{
		modeStr = {'r', '+', 'b', '\0'};
	}

	const std::filesystem::path realPath = m_root / path;

	FILE *file = nullptr;

	const auto realPathStr = realPath.generic_string();

#if HUSH_PLATFORM_WIN
	if (const errno_t error = fopen_s(&file, realPathStr.c_str(), modeStr.data()); error != 0)
	{
		LogFormat(ELogLevel::Debug, "Error opening file: {}", error);
		return IFile::EError::FileDoesntExist;
	}
#else
	if (file = fopen(realPathStr.c_str(), modeStr.data()); file == nullptr)
	{
		LogFormat(ELogLevel::Debug, "Error opening file: {}", errno);
		return IFile::EError::FileDoesntExist;
	}
#endif

// Get file size and last modified time.
// Prefer std::filesystem::file_size where available, but on Emscripten
// and in cases where filesystem fails, use stat() to avoid relying on
// fseek/ftell variants that can cause symbol/signature mismatches in WASM builds.
#if HUSH_PLATFORM_EMSCRIPTEN
	struct stat result{};
	if (stat(realPathStr.c_str(), &result) != 0)
	{
		// Close the FILE* before returning the error.
		fclose(file);
		return IFile::EError::OperationNotSupported;
	}
	std::size_t size = static_cast<std::size_t>(result.st_size);
#else
	std::error_code ec;
	std::size_t size = std::filesystem::file_size(realPath, ec);
	struct stat result{};
	if (ec)
	{
		// If filesystem query fails, fallback to stat() and log the reason.
		LogFormat(ELogLevel::Debug, "Error getting file size via std::filesystem: {}", ec.message());
		if (stat(realPathStr.c_str(), &result) != 0)
		{
			fclose(file);
			return IFile::EError::OperationNotSupported;
		}
		size = static_cast<std::size_t>(result.st_size);
	}
	else
	{
		// We still need the last modified time; use stat() for that.
		if (stat(realPathStr.c_str(), &result) != 0)
		{
			fclose(file);
			return IFile::EError::OperationNotSupported;
		}
	}
#endif

	FileInfo metadata{
		.path = std::move(vfsPath),
		.size = size,
		.lastModified = static_cast<std::uint64_t>(result.st_mtime),
		.mode = mode,
	};

	return std::make_unique<CFile>(file, std::move(metadata));
}

Hush::Result<std::vector<Hush::FileInfo>, Hush::IFile::EError> Hush::CFileSystem::ListPath(const std::string_view &path)
{
	// I know this is technically C++ and not C, but cross platform C path listing is a pain in the ass
	const std::filesystem::path realPath = m_root / path;
	HUSH_COND_FAIL_V(std::filesystem::exists(realPath) && std::filesystem::is_directory(realPath),
					 IFile::EError::PathDoesntExist);
	std::vector<FileInfo> result;
	for (const std::filesystem::directory_entry &entry : std::filesystem::directory_iterator(realPath))
	{
		FileInfo metadata = {.path = entry.path().generic_string(),
							 .mode = EFileOpenMode::None,
							 .flags = entry.is_directory() ? EFileFlags::Directory : EFileFlags::File};
		if (entry.path().has_extension())
		{
			std::string extensionWithDot = entry.path().extension().string();

			// Handle cases for `file.. or file.`
			if (extensionWithDot.size() > 1)
			{
				std::string rawExtension = StringUtils::ToUpper(
					StringUtils::SubstrView(extensionWithDot, 1, static_cast<int32_t>(extensionWithDot.size())));
				metadata.extension = this->ToKnownExtension(rawExtension);
			}
		}
		result.emplace_back(metadata);
	}
	return result;
}
