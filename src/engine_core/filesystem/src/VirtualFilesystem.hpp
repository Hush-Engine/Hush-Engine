/*! \file VirtualFilesystem.hpp
	\author Alan Ramirez
	\date 2024-12-07
	\brief A Virtual filesystem implementation
*/

#pragma once
#include "FileSystem.hpp"
#include "IFile.hpp"
#include "Result.hpp"
#include <string_view>
#include <vector>
#include <optional>

namespace Hush
{
	/// The VFS allows to manage paths as virtual locations (that might not refer to the real filesystem).
	/// For instance, to access an asset, we could use rs://myasset.txt, instead of referring to the physical location
	/// of the file. This, while introducing overhead, allows to manage more complex scenarios such as
	/// having the game files in a ZIP or a tar file. Or some other format.
	/// To use it, you must instantiate it
	class VirtualFilesystem final
	{
		struct MountPoint
		{
			std::string path;
			std::unique_ptr<IFileSystem> filesystem;
		};

		struct ResolvedPath
		{
			IFileSystem *filesystem = nullptr;
			std::string_view path;
		};

	public:
		enum class EError
		{
			None,
			FileDoesntExist,
			OperationNotSupported,
		};

		enum class EListOptions
		{
			None,
			Recursive,
		};

		VirtualFilesystem();

		~VirtualFilesystem();

		VirtualFilesystem(const VirtualFilesystem &) = delete;
		VirtualFilesystem &operator=(const VirtualFilesystem &) = delete;

		VirtualFilesystem(VirtualFilesystem &&rhs) noexcept;
		VirtualFilesystem &operator=(VirtualFilesystem &&rhs) noexcept;

		void Unmount(std::string_view virtualPath);

		std::vector<FileInfo> ListPath(std::string_view virtualPath, EListOptions options = EListOptions::None);

		Result<std::unique_ptr<IFile>, IFile::EError> OpenFile(std::string_view virtualPath,
															   EFileOpenMode mode = EFileOpenMode::Read);

		template <class T, class... Args>
		void MountFileSystem(std::string_view path, Args &&...args)
		{
			MountFileSystemInternal(path, std::make_unique<T>(std::forward<Args>(args)...));
		}
		
		// Public facing API, will call ResolveFileSystem
		Result<std::string_view, EError> ResolveVirtualPath(const std::string_view& path);

	private:
		void MountFileSystemInternal(std::string_view path, std::unique_ptr<IFileSystem> resourceLoader);

		std::optional<ResolvedPath> ResolveFileSystem(std::string_view path);

		std::vector<MountPoint> m_mountedFileSystems;
	};
} // namespace Hush
