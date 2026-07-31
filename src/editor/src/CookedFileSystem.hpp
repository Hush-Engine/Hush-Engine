/*! \file CookedFileSystem.hpp
	\brief Editor-only cooked-first IFileSystem proxy.
*/
#pragma once

#include "FileSystem.hpp"
#include "IFile.hpp"
#include "filesystem/CFileSystem/CFileSystem.hpp"

#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace Hush
{
	/// Cooked-first resolution proxy (editor only).
	///
	/// The VFS resolves a virtual path to the first prefix-matching mount with no
	/// fall-through, so cooked-first must be a single filesystem rather than two
	/// stacked mounts. On a read, this proxy checks for a cooked
	/// `.hcooked/{Fnv1a(vpath)}.hasset` output (the id scheme `CookerService` /
	/// `CookedDirectory` write) and returns it when present; otherwise it delegates
	/// to the raw file under the project root. `ResourceManager::LoadTexture` already
	/// handles the returned `HASSET_MAGIC` blob transparently.
	class CookedFileSystem final : public IFileSystem
	{
	public:
		/// @param mountPrefix The VFS mount prefix this backend is mounted at (e.g. "res://").
		/// @param projectRoot Host directory backing raw sources.
		/// @param cookedDir   Host directory holding cooked `.hasset` outputs (typically projectRoot/.hcooked).
		CookedFileSystem(std::string mountPrefix, std::string_view projectRoot, std::string_view cookedDir);

		Result<std::unique_ptr<IFile>, IFile::EError> OpenFile(std::filesystem::path vfsPath,
															   std::filesystem::path path, EFileOpenMode mode) override;

		Result<std::vector<FileInfo>, IFile::EError> ListPath(const std::string_view &path) override;

		[[nodiscard]]
		std::optional<std::filesystem::path> GetHostPath(const std::filesystem::path &rel) const override;

	private:
		std::string m_mountPrefix;
		std::filesystem::path m_projectRoot;
		std::filesystem::path m_cookedDir;
		CFileSystem m_raw;
	};
} // namespace Hush
