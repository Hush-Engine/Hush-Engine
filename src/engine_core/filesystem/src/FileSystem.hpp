/*! \file FileSystem.hpp
	\author Alan Ramirez
	\date 2024-11-17
	\brief Resource Source interface
*/

#pragma once
#include "IFile.hpp"
#include "Result.hpp"
#include "crypto/Hashing.hpp"

#include <cstddef>
#include <filesystem>
#include <functional>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

namespace Hush
{
	/// A resource loader is an interface meant to be implemented by classes that
	/// load things from a specific source (i.e. a ZIP file). This means that they must implement a way
	/// to load resources from an underlying sources, both in an async and a sync way.
	/// Some resource sources do not support a
	/// Resource sources must live until all of their children are deleted.
	class IFileSystem
	{
	public:
		/// Errors when loading data.
		enum class EError
		{
			None,
			PathDoesntExist,
			NotSupported,
			CannotRead,
		};

		/// Callback to be called when data is loaded. Raw data is owned by the Resource Source, not by the caller.
		using AsyncCallback = std::function<void(Result<std::span<std::byte>, EError>)>;

		IFileSystem() = default;

		IFileSystem(const IFileSystem &) = delete;
		IFileSystem(IFileSystem &&) = default;
		IFileSystem &operator=(const IFileSystem &) = delete;
		IFileSystem &operator=(IFileSystem &&) = default;

		virtual ~IFileSystem() = default;

		/// Opens a file from a specific path.
		/// @param path Path of the file to open
		/// @param mode Mode to open the file
		/// @return A result with a pointer to the file.
		virtual Result<std::unique_ptr<IFile>, IFile::EError> OpenFile(std::filesystem::path vfsPath,
																	   std::filesystem::path path,
																	   EFileOpenMode mode = EFileOpenMode::Read) = 0;

		/// Lists all the contents of a specific path
		virtual Result<std::vector<FileInfo>, IFile::EError> ListPath(const std::string_view &path) = 0;

		/// Returns the host (OS) path backing a mount-relative path, when this filesystem
		/// is backed by the real filesystem. Virtual backends (e.g. pak bundles) return
		/// nullopt. Used by VirtualFilesystem::ResolveHostPath.
		[[nodiscard]]
		virtual std::optional<std::filesystem::path> GetHostPath(const std::filesystem::path &rel) const
		{
			(void)rel;
			return std::nullopt;
		}

		EFileExtension ToKnownExtension(const std::string_view &extensionUppercase)
		{
			switch (Hashing::Fnv1a(extensionUppercase))
			{
			case Hashing::Fnv1a("PNG"):
				return EFileExtension::PNG;
			case Hashing::Fnv1a("META"):
				return EFileExtension::META;
			case Hashing::Fnv1a("JPEG"):
			case Hashing::Fnv1a("JPG"):
				return EFileExtension::JPEG;
			case Hashing::Fnv1a("TXT"):
				return EFileExtension::TXT;
			case Hashing::Fnv1a("PDF"):
				return EFileExtension::PDF;
			case Hashing::Fnv1a("CS"):
				return EFileExtension::CSHARP;
			case Hashing::Fnv1a("CPP"):
			case Hashing::Fnv1a("HPP"):
				return EFileExtension::CPP;
			case Hashing::Fnv1a("GLB"):
				return EFileExtension::GLB;
			case Hashing::Fnv1a("GLTF"):
				return EFileExtension::GLTF;
			case Hashing::Fnv1a("FBX"):
				return EFileExtension::FBX;
			case Hashing::Fnv1a("SLANG"):
				return EFileExtension::SLANG;
			case Hashing::Fnv1a("HMETA"):
				return EFileExtension::HMETA;
			case Hashing::Fnv1a("HASSET"):
				return EFileExtension::HASSET;
			case Hashing::Fnv1a("HSHADER"):
				return EFileExtension::HSHADER;
			case Hashing::Fnv1a("HUSHPAK"):
				return EFileExtension::HUSHPAK;
			default:
				return EFileExtension::UNKNOWN;
			}
		}
	};
} // namespace Hush
