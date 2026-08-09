#include "AssetCooker.hpp"
#include "Cookers/ImageCooker.hpp"
#include "Cookers/ShaderCooker.hpp"
#include "HAsset.hpp"
#include "HushPak.hpp"
#include "crypto/Hashing.hpp"
#include "Result.hpp"
#include "Logger.hpp"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <iterator>
#include <string>
#include <zstd.h>

namespace Hush
{

	AssetCooker::AssetCooker() = default;
	AssetCooker::~AssetCooker() = default;
	AssetCooker::AssetCooker(AssetCooker &&) noexcept = default;
	AssetCooker &AssetCooker::operator=(AssetCooker &&) noexcept = default;

	void AssetCooker::RegisterBuiltins()
	{
		RegisterCooker(std::make_unique<ImageCooker>());
		RegisterCooker(std::make_unique<ShaderCooker>());
	}

	void AssetCooker::RegisterCooker(std::unique_ptr<ICooker> cooker)
	{
		m_cookers.push_back(std::move(cooker));
	}

	ICooker *AssetCooker::FindCooker(EFileExtension ext) const
	{
		for (const auto &c : m_cookers)
		{
			for (auto supported : c->SupportedExtensions())
			{
				if (supported == ext)
				{
					return c.get();
				}
			}
		}
		return nullptr;
	}

	EFileExtension AssetCooker::ExtensionFromPath(const std::filesystem::path &path)
	{
		std::string ext = path.extension().string();
		if (!ext.empty() && ext.front() == '.')
		{
			ext.erase(ext.begin());
		}
		std::transform(ext.begin(), ext.end(), ext.begin(),
					   [](unsigned char ch) { return static_cast<char>(std::toupper(ch)); });

		if (ext == "PNG")
			return EFileExtension::PNG;
		if (ext == "JPG" || ext == "JPEG")
			return EFileExtension::JPEG;
		if (ext == "SLANG")
			return EFileExtension::SLANG;
		if (ext == "HMETA")
			return EFileExtension::HMETA;
		if (ext == "HASSET")
			return EFileExtension::HASSET;
		if (ext == "HSHADER")
			return EFileExtension::HSHADER;
		if (ext == "HUSHPAK")
			return EFileExtension::HUSHPAK;
		return EFileExtension::UNKNOWN;
	}
	Result<std::vector<std::byte>, ECookError> AssetCooker::CookToBlob(std::span<const std::byte> input,
																	   EFileExtension sourceExtension,
																	   const HMeta &meta, const CookContext &ctx)
	{
		ICooker *cooker = FindCooker(sourceExtension);
		if (cooker == nullptr)
		{
			return ECookError::UnsupportedFormat;
		}

		// Cook into uncompressed payload
		auto cookResult = cooker->Cook(input, meta, ctx);
		if (cookResult.has_error())
		{
			return cookResult.error();
		}

		auto &result = cookResult.value();

		// Build the HAsset
		HAsset asset;
		asset.header.format = result.format;
		asset.header.uncompressedSize = result.payload.size();
		asset.header.contentHash = Hashing::Fnv1a(reinterpret_cast<const char *>(result.payload.data()),
												  static_cast<uint32_t>(result.payload.size()));
		asset.extra = std::move(result.extra);
		asset.payload = std::move(result.payload);

		// Apply compression
		if (meta.compression == ECompressionFormat::Zstd)
		{
			size_t compressedBound = ZSTD_compressBound(asset.payload.size());
			std::vector<std::byte> compressed(compressedBound);

			size_t compressedSize = ZSTD_compress(compressed.data(), compressedBound, asset.payload.data(),
												  asset.payload.size(), ZSTD_defaultCLevel());
			if (ZSTD_isError(compressedSize))
			{
				LogFormat(ELogLevel::Error, "ZSTD compression failed: {}", ZSTD_getErrorName(compressedSize));
				return ECookError::CompressFailed;
			}

			compressed.resize(compressedSize);
			asset.header.compression = ECompressionFormat::Zstd;
			asset.header.compressedSize = compressedSize;
			asset.payload = std::move(compressed);
		}
		else
		{
			asset.header.compression = ECompressionFormat::None;
			asset.header.compressedSize = asset.header.uncompressedSize;
		}

		std::vector<std::byte> blob;
		HAsset::Write(blob, asset);
		return blob;
	}

	Result<void, ECookError> AssetCooker::CookDirectory(const std::filesystem::path &contentDir,
														ECompressionFormat compression)
	{
		std::error_code ec;
		if (!std::filesystem::exists(contentDir, ec))
		{
			return ECookError::FileNotFound;
		}

		const std::filesystem::path cookedDir = contentDir / ".hcooked";
		std::filesystem::create_directories(cookedDir, ec);

		for (const auto &entry : std::filesystem::recursive_directory_iterator(contentDir, ec))
		{
			if (!entry.is_regular_file())
			{
				continue;
			}

			const std::filesystem::path &path = entry.path();

			// Skip our own outputs and the .hmeta sidecars themselves.
			if (path.extension() == ".hmeta" || path.parent_path().filename() == ".hcooked")
			{
				continue;
			}

			const EFileExtension ext = ExtensionFromPath(path);
			ICooker *cooker = FindCooker(ext);
			if (cooker == nullptr)
			{
				continue;
			}

			const std::string relVPath = std::filesystem::relative(path, contentDir, ec).generic_string();

			// Load an existing .hmeta, or seed + persist a default one (refresh step).
			std::filesystem::path metaPath = path;
			metaPath += ".hmeta";
			HMeta meta;
			std::ifstream metaFile(metaPath);
			if (metaFile)
			{
				const std::string json((std::istreambuf_iterator<char>(metaFile)), std::istreambuf_iterator<char>());
				auto parsed = HMeta::FromJson(json);
				meta = parsed.has_value() ? parsed.value() : cooker->DefaultMeta(ext, relVPath);
			}
			else
			{
				meta = cooker->DefaultMeta(ext, relVPath);
				if (auto json = meta.ToJson(); json.has_value())
				{
					std::ofstream(metaPath) << json.value();
				}
			}
			meta.compression = compression;
			if (meta.id == 0)
			{
				meta.id = Hashing::Fnv1a(relVPath);
			}

			// Read source bytes.
			std::ifstream srcFile(path, std::ios::binary | std::ios::ate);
			if (!srcFile)
			{
				continue;
			}
			const auto srcSize = static_cast<size_t>(srcFile.tellg());
			srcFile.seekg(0);
			std::vector<std::byte> srcData(srcSize);
			srcFile.read(reinterpret_cast<char *>(srcData.data()), static_cast<std::streamsize>(srcSize));

			CookContext ctx;
			ctx.sourceVPath = relVPath;

			auto blob = CookToBlob(srcData, ext, meta, ctx);
			if (blob.has_error())
			{
				LogFormat(ELogLevel::Error, "AssetCooker: failed to cook {}", relVPath);
				return blob.error();
			}

			const std::filesystem::path cookedPath = cookedDir / (std::to_string(meta.id) + ".hasset");
			std::ofstream outFile(cookedPath, std::ios::binary);
			if (!outFile)
			{
				LogFormat(ELogLevel::Error, "AssetCooker: cannot open cooked output {}", cookedPath.string());
				return ECookError::WriteFailed;
			}
			outFile.write(reinterpret_cast<const char *>(blob.value().data()),
						  static_cast<std::streamsize>(blob.value().size()));
			if (!outFile)
			{
				// Don't leave a truncated .hasset behind for the pak builder to pick up.
				outFile.close();
				std::error_code removeEc;
				std::filesystem::remove(cookedPath, removeEc);
				LogFormat(ELogLevel::Error, "AssetCooker: failed to write cooked output {}", cookedPath.string());
				return ECookError::WriteFailed;
			}
		}

		return Success();
	}

	Result<void, ECookError> AssetCooker::BuildPak(std::span<const PakInput> inputs, IFile &out)
	{
		std::vector<std::byte> pakBuffer;
		HushPak::Build(pakBuffer, inputs);

		auto writeResult = out.Write(pakBuffer);
		if (writeResult.has_error())
		{
			return ECookError::Unknown;
		}

		return Success();
	}

	Result<void, ECookError> AssetCooker::BuildPakFromCookedDir(const std::filesystem::path &contentDir, IFile &out)
	{
		std::error_code ec;
		if (!std::filesystem::exists(contentDir, ec))
		{
			return ECookError::FileNotFound;
		}

		const std::filesystem::path cookedDir = contentDir / ".hcooked";

		std::vector<PakInput> inputs;
		std::vector<std::vector<std::byte>> blobStore;

		for (const auto &entry : std::filesystem::recursive_directory_iterator(contentDir, ec))
		{
			if (!entry.is_regular_file())
			{
				continue;
			}

			const std::filesystem::path &metaPath = entry.path();
			if (metaPath.extension() != ".hmeta" || metaPath.parent_path().filename() == ".hcooked")
			{
				continue;
			}

			// Derive the source path and virtual path from the .hmeta location.
			std::filesystem::path sourcePath = metaPath;
			sourcePath.replace_extension("");
			std::string relVPath = std::filesystem::relative(sourcePath, contentDir, ec).generic_string();

			// Read .hmeta to get the cooked output id.
			std::ifstream metaFile(metaPath);
			if (!metaFile)
			{
				LogFormat(ELogLevel::Error, "AssetCooker: cannot read .hmeta {}", metaPath.string());
				continue;
			}
			const std::string json((std::istreambuf_iterator<char>(metaFile)), std::istreambuf_iterator<char>());
			auto parsed = HMeta::FromJson(json);
			if (parsed.has_error())
			{
				LogFormat(ELogLevel::Error, "AssetCooker: invalid .hmeta {}", metaPath.string());
				continue;
			}

			const uint32_t id = parsed.value().id;
			std::filesystem::path cookedPath = cookedDir / (std::to_string(id) + ".hasset");
			if (!std::filesystem::exists(cookedPath, ec))
			{
				LogFormat(ELogLevel::Error, "AssetCooker: missing cooked output for {}", relVPath);
				continue;
			}

			std::ifstream cookedFile(cookedPath, std::ios::binary | std::ios::ate);
			if (!cookedFile)
			{
				LogFormat(ELogLevel::Error, "AssetCooker: cannot read cooked output {}", cookedPath.string());
				continue;
			}
			const auto cookedSize = static_cast<size_t>(cookedFile.tellg());
			cookedFile.seekg(0);
			blobStore.emplace_back(cookedSize);
			cookedFile.read(reinterpret_cast<char *>(blobStore.back().data()),
							static_cast<std::streamsize>(cookedSize));

			inputs.push_back({.virtualPath = std::move(relVPath), .data = blobStore.back()});
		}

		if (inputs.empty())
		{
			LogFormat(ELogLevel::Error, "AssetCooker: no cooked outputs found in {}", contentDir.string());
			return ECookError::FileNotFound;
		}

		return BuildPak(inputs, out);
	}

} // namespace Hush
