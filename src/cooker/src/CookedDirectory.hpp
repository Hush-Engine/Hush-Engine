#pragma once

#include "AssetCooker.hpp"
#include "HMeta.hpp"

namespace Hush
{
class VirtualFilesystem;
}
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace Hush
{

/// Manages the lifecycle of .hmeta + .hcooked/ outputs in a project directory.
/// Handles startup reconciliation, recook-on-change, and orphan cleanup.
class CookedDirectory
{
public:
	CookedDirectory();
	~CookedDirectory();

	CookedDirectory(const CookedDirectory &) = delete;
	CookedDirectory &operator=(const CookedDirectory &) = delete;

	/// Initialize with the project root and register built-in cookers.
	void Init(std::filesystem::path projectRoot, VirtualFilesystem *vfs);

	/// Scan .hmeta files on startup, recook any stale/missing outputs.
	void Reconcile();

	/// Handle a file watcher event:
	/// - Source file modified/added → recook
	/// - .hmeta modified → recook
	/// - .hmeta removed → delete cooked output
	/// @return true if the event was handled (cooked output changed).
	bool HandleFileEvent(const std::filesystem::path &path, bool isAdded, bool isRemoved, bool isModified);

	/// Access the underlying cooker.
	AssetCooker &GetCooker() { return m_cooker; }

private:
	struct TrackedAsset
	{
		std::filesystem::path sourcePath;
		std::filesystem::path metaPath;
		std::filesystem::path cookedPath;
		uint64_t sourceHash = 0;
	};

	void CookAsset(const TrackedAsset &asset);
	void RemoveCooked(const TrackedAsset &asset);
	void ScanDirectory();

	std::filesystem::path m_projectRoot;
	std::filesystem::path m_cookedDir;
	VirtualFilesystem *m_vfs = nullptr;
	AssetCooker m_cooker;
	std::vector<TrackedAsset> m_assets;
};

} // namespace Hush
