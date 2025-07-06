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
#include "crypto/Hashing.hpp"
#include "Result.hpp"

namespace Hush
{
	enum class EFileOpenMode
	{
		/// @brief The file has not been opened yet
		None = 0,
		Read,
		Write,
		ReadWrite,
	};

	enum class EFileFlags : uint16_t {
		Directory,
		File,
		Metadata
	};

	enum class EFileExtension : uint32_t { // All as uppercase to normalize hashing
		UNKWOWN,
		HUSH_HASHED_ENUM_ENTRY(PNG),
		HUSH_HASHED_ENUM_ENTRY(META),
		HUSH_HASHED_ENUM_ENTRY(JPEG),
		HUSH_HASHED_ENUM_ENTRY(TXT),
		HUSH_HASHED_ENUM_ENTRY(PDF),
		HUSH_HASHED_ENUM_ENTRY(CSHARP),
		HUSH_HASHED_ENUM_ENTRY(CPP),
		HUSH_HASHED_ENUM_ENTRY(GLB),
		HUSH_HASHED_ENUM_ENTRY(GLTF),
		HUSH_HASHED_ENUM_ENTRY(FBX),
	};

	/// Metadata for a file.
	struct FileInfo
	{
		std::filesystem::path path;
		std::size_t size;
		std::uint64_t lastModified;
		EFileOpenMode mode = EFileOpenMode::None;
		EFileFlags flags;
		EFileExtension extension;
		
		[[nodiscard]] inline bool IsCodeFile() const {
			return this->extension == EFileExtension::CPP || this->extension == EFileExtension::CSHARP;
		}

		[[nodiscard]] inline bool ShouldGenerateMetaFile() const {
			return this->flags != EFileFlags::Directory && this->extension != EFileExtension::UNKWOWN && this->extension != EFileExtension::PDF && this->extension != EFileExtension::TXT && !this->IsCodeFile();
		}
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
		virtual const FileInfo &GetFileInfo() const = 0;

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
