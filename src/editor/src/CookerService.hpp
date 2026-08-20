#pragma once

#include "AssetCooker.hpp"
#include "VirtualFilesystem.hpp"
#include <filesystem>
#include <string>

namespace Hush
{

	class FileWatcher;

	/// EditorApp-owned service that owns an AssetCooker and provides a high-level
	/// API for the editor import pipeline.
	class CookerService
	{
	public:
		CookerService();
		~CookerService();

		// Non-copyable and non-movable: held by unique_ptr from EditorApp, so the object
		// itself never moves (only the owning pointer does).
		CookerService(const CookerService &) = delete;
		CookerService &operator=(const CookerService &) = delete;
		CookerService(CookerService &&) = delete;
		CookerService &operator=(CookerService &&) = delete;

		/// Initialize with the VFS and the project root path.
		/// @param vfs  The engine's virtual filesystem.
		/// @param projectRoot  OS path to the project content directory (HUSH_DEFAULT_PROJECT_DIR).
		void Init(VirtualFilesystem *vfs, std::filesystem::path projectRoot,
				  std::pmr::memory_resource *allocator = nullptr);

		/// Import a file dropped onto the editor.
		/// Copies the source file under res://, creates .hmeta, and cooks it.
		/// @param sourcePath  OS path to the dropped file.
		void ImportFile(const std::filesystem::path &sourcePath);

		/// Cook a source file (must already be under res:// with a .hmeta sidecar).
		/// @param vpath  Virtual path like "res://textures/stone.png".
		void CookSource(const std::string_view vpath);

		/// Access the underlying AssetCooker.
		AssetCooker &GetCooker()
		{
			return m_cooker;
		}

		/// Optionally supply the file watcher so the service can suppress the watcher
		/// events caused by its own writes (copied source + .hmeta) and avoid double cooks.
		void SetFileWatcher(FileWatcher *watcher)
		{
			m_watcher = watcher;
		}

	private:
		AssetCooker m_cooker;
		VirtualFilesystem *m_vfs = nullptr;
		// Our allocator
		std::pmr::memory_resource *m_frameAllocator = nullptr;
		std::filesystem::path m_projectRoot;
		FileWatcher *m_watcher = nullptr;
	};

} // namespace Hush
