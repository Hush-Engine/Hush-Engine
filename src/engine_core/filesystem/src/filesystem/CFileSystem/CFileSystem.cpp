/*! \file FsResourceSource.cpp
	\author Alan Ramirez
	\date 2024-11-17
	\brief Resource Source interface that uses the filesystem (C API)
*/
#include "CFileSystem.hpp"

#include "CFile.hpp"

#include <Logger.hpp>
#include <array>

Hush::CFileSystem::CFileSystem(std::string_view root)
	: mRoot(root)
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

	const std::filesystem::path realPath = mRoot / path;

	FILE *file = nullptr;

	const auto realPathStr = realPath.generic_string();

	if (const errno_t error = fopen_s(&file, realPathStr.c_str(), modeStr.data()); error != 0)
	{
		// TODO: cover all the errors.
		LogFormat(ELogLevel::Debug, "Error opening file: {}", error);
		return IFile::EError::FileDoesntExist;
	}

	// Get file size
	fseek(file, 0, SEEK_END);
	const std::size_t size = ftell(file);

	// Reset file
	fseek(file, 0, SEEK_SET);

	// Get last modified
	struct stat result
	{
	};
	if (stat(realPathStr.c_str(), &result) != 0)
	{
		return IFile::EError::OperationNotSupported;
	}

	FileMetadata metadata{
		.path = std::move(vfsPath),
		.size = size,
		.lastModified = static_cast<std::uint64_t>(result.st_mtime),
		.mode = mode,
	};

	return std::make_unique<CFile>(file, std::move(metadata));
}