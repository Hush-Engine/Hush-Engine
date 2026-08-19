#include "CookerService.hpp"
#include "FileWatcher.hpp"
#include "HMeta.hpp"
#include "IFile.hpp"
#include "crypto/Hashing.hpp"
#include "Logger.hpp"
#include "serialization/Formats/JsonSerializer.hpp"
#include <filesystem>
#include <fstream>
#include <memory_resource>

namespace Hush
{

	CookerService::CookerService()
	{
		m_cooker.RegisterBuiltins();
	}

	CookerService::~CookerService() = default;

	void CookerService::Init(VirtualFilesystem *vfs, std::filesystem::path projectRoot, std::pmr::memory_resource* allocator)
	{
		m_vfs = vfs;
		m_projectRoot = std::move(projectRoot);
		m_frameAllocator = allocator;
	}

	void CookerService::ImportFile(const std::filesystem::path &sourcePath)
	{
		if (m_vfs == nullptr)
		{
			LogFormat(ELogLevel::Error, "CookerService: not initialized");
			return;
		}

		// Determine destination under res://
		std::string filename = sourcePath.filename().string();
		std::filesystem::path destPath = m_projectRoot / filename;

		// Suppress the watcher events our own copy + .hmeta write will cause, so the
		// watcher-driven pipeline doesn't redundantly re-cook what we cook here directly.
		if (m_watcher != nullptr)
		{
			m_watcher->SuppressPath(destPath);
		}

		// Copy file
		std::error_code ec;
		if (!std::filesystem::copy_file(sourcePath, destPath, std::filesystem::copy_options::overwrite_existing, ec))
		{
			LogFormat(ELogLevel::Error, "CookerService: failed to copy {} to {}: {}", sourcePath.string(),
					  destPath.string(), ec.message());
			return;
		}

		// Create .hmeta sidecar
		std::string vpath = std::string("res://") + filename;
		uint32_t id = Hashing::Fnv1a(vpath);

		// Determine extension for DefaultMeta
		EFileExtension fileExt = AssetCooker::ExtensionFromPath(destPath);

		// Try to get DefaultMeta from a registered cooker
		HMeta meta;
		meta.id = id;
		meta.assetType = "unknown";
		meta.compression = ECompressionFormat::Zstd;

		ICooker *cooker = m_cooker.FindCooker(fileExt);
		if (cooker != nullptr)
		{
			meta = cooker->DefaultMeta(fileExt, vpath);
		}

		// Write .hmeta
		std::filesystem::path metaPath = destPath;
		metaPath += ".hmeta";
		if (m_watcher != nullptr)
		{
			m_watcher->SuppressPath(metaPath);
		}
		auto jsonResult = meta.ToJson();
		if (jsonResult.has_error())
		{
			LogFormat(ELogLevel::Error, "CookerService: failed to serialize .hmeta for {}", filename);
			return;
		}

		std::ofstream metaFile(metaPath);
		if (!metaFile)
		{
			LogFormat(ELogLevel::Error, "CookerService: failed to write .hmeta for {}", filename);
			return;
		}
		metaFile << jsonResult.value();
		// Flush + release the file before cooking: CookSource re-opens this .hmeta to read it,
		// and an unflushed ofstream would leave it empty/partial on disk (invalid .hmeta).
		metaFile.close();

		LogFormat(ELogLevel::Info, "CookerService: imported {} -> {}", sourcePath.filename().string(),
				  metaPath.string());

		// Cook the source
		CookSource(vpath);
	}

	void CookerService::CookSource(const std::string_view vpath)
	{
		if (m_vfs == nullptr)
		{
			return;
		}

		// Resolve the virtual path to its backing OS (source) path.
		auto resolved = m_vfs->ResolveHostPath(vpath);
		if (resolved.has_error())
		{
			LogFormat(ELogLevel::Error, "CookerService: cannot resolve {}", vpath);
			return;
		}

		std::filesystem::path osPath = resolved.value();

		// Read source file
		std::ifstream inFile(osPath, std::ios::binary | std::ios::ate);
		if (!inFile)
		{
			LogFormat(ELogLevel::Error, "CookerService: cannot read {}", osPath.string());
			return;
		}
		auto fileSize = static_cast<size_t>(inFile.tellg());
		inFile.seekg(0);
		std::vector<std::byte> fileData(fileSize);
		inFile.read(reinterpret_cast<char *>(fileData.data()), static_cast<std::streamsize>(fileSize));

		// Read .hmeta
		std::filesystem::path metaPath = osPath;
		metaPath += ".hmeta";
		std::ifstream metaFile(metaPath);
		if (!metaFile)
		{
			LogFormat(ELogLevel::Error, "CookerService: no .hmeta for {}", vpath);
			return;
		}
		std::string metaJson((std::istreambuf_iterator<char>(metaFile)), std::istreambuf_iterator<char>());

		auto metaResult = HMeta::FromJson(metaJson);
		if (metaResult.has_error())
		{
			LogFormat(ELogLevel::Error, "CookerService: invalid .hmeta for {}", vpath);
			return;
		}

		HMeta meta = metaResult.value();

		CookContext ctx;
		ctx.sourceVPath = std::string(vpath);
		ctx.frameAllocator = m_frameAllocator;

		auto cookResult = m_cooker.CookToBlob(fileData, AssetCooker::ExtensionFromPath(osPath), meta, ctx);
		if (cookResult.has_error())
		{
			LogFormat(ELogLevel::Error, "CookerService: failed to cook {}", vpath);
			return;
		}

		// Write .hcooked/{id}.hasset
		std::filesystem::path cookedDir = m_projectRoot / ".hcooked";
		std::error_code ec;
		std::filesystem::create_directories(cookedDir, ec);

		std::string cookedName = std::to_string(meta.id) + ".hasset";
		std::filesystem::path cookedPath = cookedDir / cookedName;

		std::ofstream outFile(cookedPath, std::ios::binary);
		outFile.write(reinterpret_cast<const char *>(cookResult.value().data()), cookResult.value().size());

		LogFormat(ELogLevel::Info, "CookerService: cooked {} -> {}", vpath, cookedPath.string());
	}

} // namespace Hush
