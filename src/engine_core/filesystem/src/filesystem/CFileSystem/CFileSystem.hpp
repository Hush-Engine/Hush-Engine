/*! \file CFileSystem.hpp
    \author Alan Ramirez
    \date 2024-11-17
    \brief Resource Source interface that uses the filesystem (C API)
*/

#pragma once
#include "FileSystem.hpp"
#include <cstdio>
#include <filesystem>
#include <string>
#include <unordered_map>

namespace Hush
{
    /// Loads data using C APIs.
    /// This, however, is not efficient as it loads the whole file into memory.
    /// For more complex scenarios, look at memory mapped sources.
    class CFileSystem final : public IFileSystem
    {
    public:
        CFileSystem(std::string_view root);

        CFileSystem(const CFileSystem &) = delete;

        CFileSystem(CFileSystem &&rhs) noexcept
            : mLoadedFiles(std::move(rhs.mLoadedFiles))
        {
        }

        CFileSystem &operator=(const CFileSystem &) = delete;

        CFileSystem &operator=(CFileSystem &&rhs) noexcept
        {
            mLoadedFiles = std::move(rhs.mLoadedFiles);
            return *this;
        }

        ~CFileSystem() override = default;

        Result<std::unique_ptr<IFile>, IFile::EError> OpenFile(std::filesystem::path vfsPath,
                                                               std::filesystem::path path, EFileOpenMode mode) override;

    private:
        std::filesystem::path mRoot;
        std::unordered_map<std::byte *, std::unique_ptr<std::byte[]>> mLoadedFiles;
    };
} // namespace Hush