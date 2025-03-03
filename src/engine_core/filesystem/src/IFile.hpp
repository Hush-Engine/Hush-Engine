/*! \file IFile.hpp
    \author Alan Ramirez
    \date 2024-12-22
    \brief A file for the VFS
*/

#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <filesystem>

#include "Result.hpp"

namespace Hush
{
    enum class EFileOpenMode
    {
        Read,
        Write,
        ReadWrite
    };

    /// Metadata for a file.
    struct FileMetadata
    {
        std::filesystem::path path;
        std::size_t size;
        std::uint64_t lastModified;
        EFileOpenMode mode;
    };

    /// File interface for the VFS.
    /// A file is a resource that maps to a specific path in the VFS.
    /// Files support reading, writing, and reading metadata.
    /// Not all the VFS implementations will support all the operations, and, in those cases,
    /// the implementation will return a "OperationNotSupported" error.
    /// TODO: async interfaces.
    class IFile
    {
    public:
        /// Errors that can occur when reading or writing a file.
        enum class EError
        {
            None,
            FileDoesntExist,
            OperationNotSupported,
            NotWritable,
            NotReadable,
            CannotRead,
            CannotWrite,
            PathDoesntExist
        };
        template <typename T>
        using Result = Hush::Result<T, EError>;

        IFile() = default;
        virtual ~IFile() = default;

        [[nodiscard]]
        virtual const FileMetadata &GetMetadata() const = 0;

        /// Writes the file.
        /// @param data Data to write to the file.
        /// @return Result with the error that occurred.
        [[nodiscard]]
        virtual Result<void> Write(std::span<const std::byte> data) = 0;

        /// Reads the file.
        /// @param data Span to the buffer to read the data into.
        /// @return Result with the number of bytes read.
        [[nodiscard]]
        virtual Result<std::size_t> Read(std::span<std::byte> data) = 0;

        /// Specialized read function that reads a specific amount of bytes.
        /// This is used when reading mmaped files. This might not be supported by all implementations.
        /// @param size Size to read.
        /// @return Result with the data read.
        [[nodiscard]]
        virtual Result<std::span<std::byte>> Read(std::size_t size) = 0;

        /// Seeks to a specific position in the file.
        /// @param position Position to seek to.
        /// @return Result with the error that occurred.
        [[nodiscard]]
        virtual Result<void> Seek(std::size_t position) = 0;

        /// Closes the file.
        virtual void Close() = 0;
    };

} // namespace Hush