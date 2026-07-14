#include "AssetCooker.hpp"
#include "HAsset.hpp"
#include "HushPak.hpp"
#include "Logger.hpp"
#include <cstdio>
#include <cstring>
#include <deque>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

static void PrintUsage(const char *argv0)
{
	std::fprintf(stderr, "Hush Cooker CLI v0.1\n");
	std::fprintf(stderr, "Usage:\n");
	std::fprintf(stderr, "  %s cook --content-dir <dir> --out <bundle> [--compression zstd|none]\n", argv0);
	std::fprintf(stderr, "  %s cook-file <in> --meta <hmeta> --out <blob>\n", argv0);
	std::fprintf(stderr, "  %s inspect <bundle|blob>\n", argv0);
	// NOTE: a standalone `pack <hcooked-dir>` command is intentionally omitted: loose
	// .hcooked outputs are keyed by content id and carry no source virtual path, so a
	// bundle built from them could not be looked up at runtime. Use `cook` end-to-end.
}

int main(int argc, char **argv)
{
	if (argc < 2)
	{
		PrintUsage(argv[0]);
		return 1;
	}

	const std::string cmd = argv[1];

	if (cmd == "cook" && argc >= 5)
	{
		std::string contentDir, outPath, compression("zstd");
		for (int i = 2; i < argc; ++i)
		{
			if (std::strcmp(argv[i], "--content-dir") == 0 && i + 1 < argc)
				contentDir = argv[++i];
			else if (std::strcmp(argv[i], "--out") == 0 && i + 1 < argc)
				outPath = argv[++i];
			else if (std::strcmp(argv[i], "--compression") == 0 && i + 1 < argc)
				compression = argv[++i];
		}

		if (contentDir.empty() || outPath.empty())
		{
			PrintUsage(argv[0]);
			return 1;
		}

		Hush::AssetCooker cooker;
		cooker.RegisterBuiltins();

		Hush::ECompressionFormat compFmt = Hush::ECompressionFormat::Zstd;
		if (compression == "none")
			compFmt = Hush::ECompressionFormat::None;

		// Walk the content directory, cook each file
		std::vector<Hush::PakInput> pakInputs;
		// PakInput::data is a non-owning span; keep the cooked blobs alive until
		// HushPak::Build runs. std::deque keeps element addresses stable across push_back.
		std::deque<std::vector<std::byte>> blobStore;
		std::error_code ec;

		for (const auto &entry : std::filesystem::recursive_directory_iterator(contentDir, ec))
		{
			if (!entry.is_regular_file())
				continue;

			auto path = entry.path();
			auto ext = path.extension().string();

			// Determine the file extension for cooker lookup
			Hush::EFileExtension fileExt = Hush::EFileExtension::UNKNOWN;
			if (ext == ".png" || ext == ".PNG")
				fileExt = Hush::EFileExtension::PNG;
			else if (ext == ".jpg" || ext == ".jpeg" || ext == ".JPEG" || ext == ".JPG")
				fileExt = Hush::EFileExtension::JPEG;
			else if (ext == ".slang" || ext == ".SLANG")
				fileExt = Hush::EFileExtension::SLANG;

			auto *cookerPtr = cooker.FindCooker(fileExt);
			if (cookerPtr == nullptr)
				continue;

			// Read file
			std::ifstream file(path, std::ios::binary | std::ios::ate);
			if (!file)
				continue;

			auto fileSize = static_cast<size_t>(file.tellg());
			file.seekg(0);
			std::vector<std::byte> fileData(fileSize);
			file.read(reinterpret_cast<char *>(fileData.data()), static_cast<std::streamsize>(fileSize));

			// Compute relative virtual path
			std::string relPath = std::filesystem::relative(path, contentDir, ec).generic_string();

			Hush::HMeta meta = cookerPtr->DefaultMeta(fileExt, relPath);
			meta.compression = compFmt;

			Hush::CookContext ctx;
			ctx.sourceVPath = relPath;

			auto cookResult = cooker.CookToBlob(fileData, fileExt, meta, ctx);
			if (cookResult.has_error())
			{
				std::fprintf(stderr, "Failed to cook %s\n", relPath.c_str());
				continue;
			}

			blobStore.push_back(std::move(cookResult.value()));
			pakInputs.push_back({.virtualPath = relPath, .data = blobStore.back()});
			std::fprintf(stdout, "Cooked: %s\n", relPath.c_str());
		}

		if (pakInputs.empty())
		{
			std::fprintf(stderr, "No files cooked.\n");
			return 1;
		}

		// Build the bundle
		std::vector<std::byte> pakBuffer;
		Hush::HushPak::Build(pakBuffer, pakInputs);

		std::ofstream outFile(outPath, std::ios::binary);
		outFile.write(reinterpret_cast<const char *>(pakBuffer.data()), pakBuffer.size());

		std::fprintf(stdout, "Bundle written: %s (%zu entries, %zu bytes)\n",
					 outPath.c_str(), pakInputs.size(), pakBuffer.size());
		return 0;
	}

	if (cmd == "cook-file" && argc >= 4)
	{
		std::string inPath, metaPath, outPath;

		// First positional arg is the input file
		for (int i = 2; i < argc; ++i)
		{
			if (argv[i][0] != '-')
			{
				if (inPath.empty())
				{
					inPath = argv[i];
					continue;
				}
				break;
			}
			if (std::strcmp(argv[i], "--meta") == 0 && i + 1 < argc)
				metaPath = argv[++i];
			else if (std::strcmp(argv[i], "--out") == 0 && i + 1 < argc)
				outPath = argv[++i];
		}

		if (inPath.empty() || outPath.empty())
		{
			PrintUsage(argv[0]);
			return 1;
		}

		Hush::AssetCooker cooker;
		cooker.RegisterBuiltins();

		std::ifstream inFile(inPath, std::ios::binary | std::ios::ate);
		if (!inFile)
		{
			std::fprintf(stderr, "Cannot open input: %s\n", inPath.c_str());
			return 1;
		}
		auto inFileSize = static_cast<size_t>(inFile.tellg());
		inFile.seekg(0);
		std::vector<std::byte> fileData(inFileSize);
		inFile.read(reinterpret_cast<char *>(fileData.data()), static_cast<std::streamsize>(inFileSize));

		Hush::HMeta meta;
		if (!metaPath.empty())
		{
			std::ifstream metaFile(metaPath);
			std::string json((std::istreambuf_iterator<char>(metaFile)),
							 std::istreambuf_iterator<char>());
			auto metaResult = Hush::HMeta::FromJson(json);
			if (metaResult.has_error())
			{
				std::fprintf(stderr, "Failed to parse .hmeta\n");
				return 1;
			}
			meta = metaResult.value();
		}

		Hush::CookContext ctx;
		ctx.sourceVPath = inPath;

		auto result = cooker.CookToBlob(fileData, Hush::AssetCooker::ExtensionFromPath(inPath), meta, ctx);
		if (result.has_error())
		{
			std::fprintf(stderr, "Failed to cook file\n");
			return 1;
		}

		std::ofstream outFile(outPath, std::ios::binary);
		outFile.write(reinterpret_cast<const char *>(result.value().data()), result.value().size());

		std::fprintf(stdout, "Cooked: %s -> %s (%zu bytes)\n", inPath.c_str(), outPath.c_str(), result.value().size());
		return 0;
	}

	if (cmd == "inspect" && argc >= 3)
	{
		std::string inPath = argv[2];
		std::ifstream file(inPath, std::ios::binary | std::ios::ate);
		if (!file)
		{
			std::fprintf(stderr, "Cannot open: %s\n", inPath.c_str());
			return 1;
		}

		auto size = file.tellg();
		file.seekg(0);
		std::vector<std::byte> data(static_cast<size_t>(size));
		file.read(reinterpret_cast<char *>(data.data()), size);

		// Sniff magic
		if (data.size() >= 4)
		{
			uint32_t magic;
			std::memcpy(&magic, data.data(), sizeof(uint32_t));

			if (magic == Hush::HUSHPAK_MAGIC)
			{
				auto pak = Hush::HushPak::Read(data);
				if (pak.has_value())
				{
					std::fprintf(stdout, "HushPak: %u entries\n", pak->header.entryCount);
					for (const auto &entry : pak->directory)
					{
						std::string_view name;
						if (entry.nameOffset + entry.nameLength <= pak->stringTable.size())
						{
							name = std::string_view(pak->stringTable.data() + entry.nameOffset, entry.nameLength);
						}
						std::fprintf(stdout, "  [%016llX] %.*s (fmt=%u, comp=%u, size=%llu)\n",
									 static_cast<unsigned long long>(entry.nameHash),
									 static_cast<int>(name.size()), name.data(),
									 static_cast<unsigned>(entry.format),
									 static_cast<unsigned>(entry.compression),
									 static_cast<unsigned long long>(entry.dataSize));
					}
				}
			}
			else if (magic == Hush::HASSET_MAGIC)
			{
				auto asset = Hush::HAsset::Read(data);
				if (asset.has_value())
				{
					std::fprintf(stdout, "HAsset: format=%u, comp=%u, raw=%llu, compressed=%llu, extra=%u\n",
								 static_cast<unsigned>(asset->header.format),
								 static_cast<unsigned>(asset->header.compression),
								 static_cast<unsigned long long>(asset->header.uncompressedSize),
								 static_cast<unsigned long long>(asset->header.compressedSize),
								 asset->header.extraSize);
				}
			}
			else
			{
				std::fprintf(stdout, "Unknown format (magic: 0x%08X)\n", magic);
			}
		}
		return 0;
	}

	PrintUsage(argv[0]);
	return 1;
}
