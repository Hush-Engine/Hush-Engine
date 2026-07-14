#include "CookedDirectory.hpp"
#include "crypto/Hashing.hpp"
#include "Logger.hpp"
#include "IFile.hpp"
#include <fstream>

namespace Hush
{

CookedDirectory::CookedDirectory()
{
	m_cooker.RegisterBuiltins();
}

CookedDirectory::~CookedDirectory() = default;

void CookedDirectory::Init(std::filesystem::path projectRoot, VirtualFilesystem *vfs)
{
	m_projectRoot = std::move(projectRoot);
	m_cookedDir = m_projectRoot / ".hcooked";
	m_vfs = vfs;

	std::error_code ec;
	std::filesystem::create_directories(m_cookedDir, ec);
}

void CookedDirectory::Reconcile()
{
	ScanDirectory();

	for (const auto &asset : m_assets)
	{
		// Check if cooked output exists and is fresh
		bool needsCook = false;

		if (!std::filesystem::exists(asset.cookedPath))
		{
			needsCook = true;
		}
		else
		{
			// Check source modification time
			std::error_code ec;
			auto srcMTime = std::filesystem::last_write_time(asset.sourcePath, ec);
			auto cookedMTime = std::filesystem::last_write_time(asset.cookedPath, ec);
			if (!ec && srcMTime > cookedMTime)
			{
				needsCook = true;
			}
		}

		if (needsCook)
		{
			CookAsset(asset);
		}
	}
}

bool CookedDirectory::HandleFileEvent(const std::filesystem::path &path, bool isAdded, bool isRemoved, bool isModified)
{
	// Check if this is a .hmeta file
	if (path.extension() == ".hmeta")
	{
		auto sourcePath = path;
		sourcePath.replace_extension("");

		// Find the tracked asset
		for (auto &asset : m_assets)
		{
			if (asset.metaPath == path)
			{
				if (isRemoved)
				{
					RemoveCooked(asset);
					return true;
				}
				// Modified or added → recook if still exists
				if (std::filesystem::exists(path))
				{
					CookAsset(asset);
					return true;
				}
				return false;
			}
		}

		// New .hmeta → add to tracking
		if (isAdded || isModified)
		{
			ScanDirectory();
			return true;
		}

		return false;
	}

	// Source file changed
	if (isAdded || isModified)
	{
		// Check if there's a corresponding .hmeta
		auto metaPath = path;
		metaPath += ".hmeta";
		if (!std::filesystem::exists(metaPath))
		{
			return false;
		}

		// Read the .hmeta to get the ID
		std::ifstream metaFile(metaPath);
		if (!metaFile)
		{
			return false;
		}
		std::string metaJson((std::istreambuf_iterator<char>(metaFile)),
							 std::istreambuf_iterator<char>());

		auto metaResult = HMeta::FromJson(metaJson);
		if (metaResult.has_error())
		{
			return false;
		}

		// Cook the source
		auto &meta = metaResult.value();
		std::ifstream srcFile(path, std::ios::binary | std::ios::ate);
		if (!srcFile)
		{
			return false;
		}
		auto srcSize = static_cast<size_t>(srcFile.tellg());
		srcFile.seekg(0);
		std::vector<std::byte> srcData(srcSize);
		srcFile.read(reinterpret_cast<char *>(srcData.data()), static_cast<std::streamsize>(srcSize));

		CookContext ctx;
		ctx.sourceVPath = std::filesystem::relative(path, m_projectRoot).generic_string();

		auto result = m_cooker.CookToBlob(srcData, AssetCooker::ExtensionFromPath(path), meta, ctx);
		if (result.has_error())
		{
			LogFormat(ELogLevel::Error, "CookedDirectory: failed to cook {}", path.string());
			return false;
		}

		// Write cooked output
		auto cookedPath = m_cookedDir / (std::to_string(meta.id) + ".hasset");
		std::ofstream outFile(cookedPath, std::ios::binary);
		outFile.write(reinterpret_cast<const char *>(result.value().data()), result.value().size());

		LogFormat(ELogLevel::Info, "CookedDirectory: cooked {} -> {}", path.filename().string(), cookedPath.string());
		return true;
	}

	if (isRemoved)
	{
		// Source removed — delete its cooked output and the orphaned .hmeta sidecar,
		// then stop tracking it.
		for (auto it = m_assets.begin(); it != m_assets.end(); ++it)
		{
			if (it->sourcePath == path)
			{
				RemoveCooked(*it);
				std::error_code ec;
				if (std::filesystem::remove(it->metaPath, ec))
				{
					LogFormat(ELogLevel::Info, "CookedDirectory: removed orphaned {}", it->metaPath.string());
				}
				m_assets.erase(it);
				return true;
			}
		}
	}

	return false;
}

void CookedDirectory::CookAsset(const TrackedAsset &asset)
{
	// Read source
	std::ifstream srcFile(asset.sourcePath, std::ios::binary | std::ios::ate);
	if (!srcFile)
	{
		LogFormat(ELogLevel::Error, "CookedDirectory: cannot read source {}", asset.sourcePath.string());
		return;
	}
	auto srcSize = static_cast<size_t>(srcFile.tellg());
	srcFile.seekg(0);
	std::vector<std::byte> srcData(srcSize);
	srcFile.read(reinterpret_cast<char *>(srcData.data()), static_cast<std::streamsize>(srcSize));

	// Read .hmeta
	std::ifstream metaFile(asset.metaPath);
	if (!metaFile)
	{
		LogFormat(ELogLevel::Error, "CookedDirectory: no .hmeta for {}", asset.sourcePath.string());
		return;
	}
	std::string metaJson((std::istreambuf_iterator<char>(metaFile)),
						 std::istreambuf_iterator<char>());

	auto metaResult = HMeta::FromJson(metaJson);
	if (metaResult.has_error())
	{
		LogFormat(ELogLevel::Error, "CookedDirectory: invalid .hmeta for {}", asset.sourcePath.string());
		return;
	}

	auto &meta = metaResult.value();
	CookContext ctx;
	ctx.sourceVPath = std::filesystem::relative(asset.sourcePath, m_projectRoot).generic_string();

	auto result = m_cooker.CookToBlob(srcData, AssetCooker::ExtensionFromPath(asset.sourcePath), meta, ctx);
	if (result.has_error())
	{
		LogFormat(ELogLevel::Error, "CookedDirectory: failed to cook {}", asset.sourcePath.string());
		return;
	}

	std::error_code ec;
	std::filesystem::create_directories(m_cookedDir, ec);

	std::ofstream outFile(asset.cookedPath, std::ios::binary);
	outFile.write(reinterpret_cast<const char *>(result.value().data()), result.value().size());

	LogFormat(ELogLevel::Info, "CookedDirectory: cooked {}", asset.sourcePath.filename().string());
}

void CookedDirectory::RemoveCooked(const TrackedAsset &asset)
{
	std::error_code ec;
	if (std::filesystem::remove(asset.cookedPath, ec))
	{
		LogFormat(ELogLevel::Info, "CookedDirectory: removed {}", asset.cookedPath.string());
	}
}

void CookedDirectory::ScanDirectory()
{
	m_assets.clear();

	// Sources deleted while the editor was closed leave orphaned .hmeta sidecars. Collect
	// them during the walk and remove them afterwards (mutating the directory mid-iteration
	// is unsafe).
	std::vector<TrackedAsset> orphanedAssets;

	std::error_code ec;
	for (const auto &entry : std::filesystem::recursive_directory_iterator(m_projectRoot, ec))
	{
		auto path = entry.path();
		if (path.extension() != ".hmeta")
		{
			continue;
		}
		if (path.parent_path().filename() == ".hcooked")
		{
			continue;
		}

		TrackedAsset asset;
		asset.metaPath = path;
		asset.sourcePath = path;
		asset.sourcePath.replace_extension("");

		// Read .hmeta to get the ID
		std::ifstream metaFile(path);
		if (!metaFile)
		{
			continue;
		}
		std::string json((std::istreambuf_iterator<char>(metaFile)),
						 std::istreambuf_iterator<char>());

		auto metaResult = HMeta::FromJson(json);
		if (metaResult.has_error())
		{
			continue;
		}

		asset.sourceHash = metaResult.value().sourceHash;
		std::string cookedName = std::to_string(metaResult.value().id) + ".hasset";
		asset.cookedPath = m_cookedDir / cookedName;

		// If the source was deleted, this .hmeta + cooked output are orphaned.
		std::error_code sourceEc;
		if (!std::filesystem::exists(asset.sourcePath, sourceEc))
		{
			orphanedAssets.push_back(std::move(asset));
			continue;
		}

		m_assets.push_back(std::move(asset));
	}

	// Remove orphaned .hmeta + cooked outputs for sources that no longer exist.
	for (const auto &orphan : orphanedAssets)
	{
		RemoveCooked(orphan);
		std::error_code removeEc;
		if (std::filesystem::remove(orphan.metaPath, removeEc))
		{
			LogFormat(ELogLevel::Info, "CookedDirectory: removed orphaned {} (source deleted)",
					  orphan.metaPath.string());
		}
	}

	// GC orphaned cooked files
	for (const auto &entry : std::filesystem::directory_iterator(m_cookedDir, ec))
	{
		bool found = false;
		auto filename = entry.path().filename().string();
		for (const auto &asset : m_assets)
		{
			if (asset.cookedPath.filename().string() == filename)
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			std::filesystem::remove(entry.path(), ec);
			LogFormat(ELogLevel::Info, "CookedDirectory: GC removed orphaned {}", entry.path().string());
		}
	}
}

} // namespace Hush
