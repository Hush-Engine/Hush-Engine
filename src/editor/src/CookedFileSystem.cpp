/*! \file CookedFileSystem.cpp */
#include "CookedFileSystem.hpp"

#include "crypto/Hashing.hpp"

#include <filesystem>
#include <system_error>
#include <utility>

namespace Hush
{

	CookedFileSystem::CookedFileSystem(std::string mountPrefix, std::string_view projectRoot,
									   std::string_view cookedDir)
		: m_mountPrefix(std::move(mountPrefix)),
		  m_projectRoot(projectRoot),
		  m_cookedDir(cookedDir),
		  m_raw(projectRoot)
	{
	}

	Result<std::unique_ptr<IFile>, IFile::EError> CookedFileSystem::OpenFile(std::filesystem::path vfsPath,
																			 std::filesystem::path path,
																			 EFileOpenMode mode)
	{
		if (mode == EFileOpenMode::Read)
		{
			// Reconstruct the full vpath from the known mount prefix + the mount-relative
			// remainder. This matches the id CookerService/CookedDirectory hash
			// (Fnv1a("res://<name>")) without depending on how a filesystem::path stringifies.
			const std::string vpath = m_mountPrefix + path.generic_string();
			const uint32_t id = Hashing::Fnv1a(vpath);

			std::filesystem::path cookedAbs = m_cookedDir / (std::to_string(id) + ".hasset");
			std::error_code ec;
			if (std::filesystem::exists(cookedAbs, ec) && !ec)
			{
				std::filesystem::path cookedRel = std::filesystem::relative(cookedAbs, m_projectRoot, ec);
				if (!ec && !cookedRel.empty())
				{
					return m_raw.OpenFile(std::move(vfsPath), std::move(cookedRel), mode);
				}
			}
		}

		// No cooked output (or non-read mode) — delegate to the raw source.
		return m_raw.OpenFile(std::move(vfsPath), std::move(path), mode);
	}

	Result<std::vector<FileInfo>, IFile::EError> CookedFileSystem::ListPath(const std::string_view &path)
	{
		return m_raw.ListPath(path);
	}

	std::optional<std::filesystem::path> CookedFileSystem::GetHostPath(const std::filesystem::path &rel) const
	{
		return m_projectRoot / rel;
	}

} // namespace Hush
