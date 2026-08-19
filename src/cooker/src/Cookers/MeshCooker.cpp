#include "MeshCooker.hpp"
#include "AssetFormat.hpp"
#include "Components/MeshReference.hpp"
#include "HMeta.hpp"
#include "Loaders/GltfLoadFunctions.hpp"
#include "Loaders/GltfLoader.hpp"
#include "ICooker.hpp"
#include "Logger.hpp"
#include "crypto/Hashing.hpp"
#include <algorithm>
#include <cstdint>
#include <cstring>
#include <magic_enum/magic_enum.hpp>
#include <vector>

std::span<const Hush::EFileExtension> Hush::MeshCooker::SupportedExtensions() const
{
	return EXTENSIONS;
}

/// Whether this cooker can handle the given file info.
bool Hush::MeshCooker::CanCook(const FileInfo &info) const
{
	return info.IsModelFile();
}

/// Seed default HMeta for a source file.
Hush::HMeta Hush::MeshCooker::DefaultMeta(EFileExtension ext, std::string_view srcVPath) const
{
	(void)ext;
	(void)srcVPath;
	HMeta result{};
	result.assetType = "model";
	result.id = Hashing::Fnv1a(srcVPath);
	// MAYBE: Perhaps in the future we might want compression for large meshes?
	result.compression = ECompressionFormat::None;
	result.outputFormat = EAssetFormat::Mesh;
	result.sourceHash = result.id;
	// The default one seems to do just fine
	result.model = {};
	return result;
}

Hush::Result<Hush::ICooker::CookResult, Hush::ECookError> Hush::MeshCooker::Cook(std::span<const std::byte> input,
																				 const HMeta &meta,
																				 const CookContext &ctx)
{
	(void)input;
	(void)meta;
	(void)ctx;

	// We have the potential GLB in memory, we need to use the GltfLoader to cook it into multiple files
	GLTFLoader::AssetHandle modelAsset{};
	modelAsset.Alloc(ctx.frameAllocator);

	GltfLoadFunctions::EError err = GLTFLoader::LoadAssetFromBinary(input, &modelAsset);
	if (err != GltfLoadFunctions::EError::None)
	{
		LogFormat(ELogLevel::Error, "Could not process file {}, internal error: {}", ctx.sourceVPath,
				  magic_enum::enum_name(err));
		modelAsset.Dispose();
		return ECookError::InvalidData;
	}

	// With the asset loaded, we need to write its VertexBuffer, IndexBuffer, Materials, Surfaces and Textures
	std::vector<Mesh::Vertex> vertexBuff;
	std::vector<uint32_t> indexBuff;
	std::vector<GLTFLoader::MaterialInfo> mats;
	std::vector<std::vector<GLTFLoader::TextureInfo>> texturesByMaterial;
	std::vector<GeoSurface> surfaces;

	GLTFLoader::FillMeshData(&modelAsset, 0, &vertexBuff, &indexBuff, &mats, &texturesByMaterial, &surfaces);

	// GLB container layout: 12-byte header followed by chunks, each prefixed by [length:uint32][type:uint32].
	// The first chunk is the JSON one; the binary (BIN) chunk follows it. The texture offsets reported by
	// FillMeshData are relative to the BIN chunk, so translate them to file-relative offsets here so the
	// runtime can seek + read the texture bytes directly, regardless of the container format.
	const auto findBinChunkStart = [](std::span<const std::byte> bytes) -> uint64_t {
		if (bytes.size() < 20)
		{
			return 0;
		}
		const uint32_t jsonChunkLength = *reinterpret_cast<const uint32_t *>(bytes.data() + 12);
		// 12 (file header) + 8 (json chunk header) + jsonChunkLength + 8 (bin chunk header)
		return 12ULL + 8ULL + static_cast<uint64_t>(jsonChunkLength) + 8ULL;
	};
	const uint64_t binChunkStart = findBinChunkStart(input);

	// Flatten the per-material texture lists into a single table. Each entry carries the owning
	// material's resource id (stamped by FillMeshData), so no gltf-index -> cooked-index mapping is needed.
	std::vector<MeshTextureInfo> textureTable;
	for (const auto &materialTextures : texturesByMaterial)
	{
		for (const GLTFLoader::TextureInfo &tex : materialTextures)
		{
			const bool alreadyAdded =
				std::any_of(textureTable.begin(), textureTable.end(), [&](const MeshTextureInfo &existing) {
					return existing.materialResource == tex.resource && existing.binding == tex.binding;
				});
			if (alreadyAdded)
			{
				continue;
			}

			MeshTextureInfo out{};
			out.materialResource = tex.resource;
			out.binding = tex.binding;
			out.offset = binChunkStart + tex.offset;
			out.size = tex.size;
			std::memcpy(&(out.name[0]), &(tex.name[0]), sizeof(out.name));
			textureTable.push_back(out);
		}
	}

	// Cook it into the asset
	CookResult result{};
	std::vector<std::byte> *payload = &result.payload;
	// We need a header with some metadata
	Header header{};
	header.vertexCount = static_cast<uint32_t>(vertexBuff.size());
	header.indexCount = static_cast<uint32_t>(indexBuff.size());
	header.surfaceCount = static_cast<uint32_t>(surfaces.size());
	header.materialCount = static_cast<uint32_t>(mats.size());
	header.textureCount = static_cast<uint32_t>(textureTable.size());
	std::memset(header.sourcePath, 0, sizeof(header.sourcePath));
	std::memcpy(header.sourcePath, ctx.sourceVPath.data(),
				std::min(ctx.sourceVPath.size(), sizeof(header.sourcePath) - 1));

	const size_t vertexBuffByteSize = (sizeof(Mesh::Vertex) * vertexBuff.size());
	const size_t indexBuffByteSize = (sizeof(uint32_t) * indexBuff.size());
	const size_t matsByteSize = (sizeof(GLTFLoader::MaterialInfo) * mats.size());
	const size_t surfacesByteSize = (sizeof(GeoSurface) * surfaces.size());
	const size_t textureTableByteSize = (sizeof(MeshTextureInfo) * textureTable.size());
	const size_t fileSize = sizeof(Header) + vertexBuffByteSize + indexBuffByteSize + matsByteSize + surfacesByteSize +
							textureTableByteSize;

	payload->resize(fileSize);
	std::byte *targetMem = payload->data();

	// Copy the header
	std::memcpy(targetMem, &header, sizeof(Header));
	targetMem += sizeof(Header);

	// VB, IB, MAT

	std::memcpy(targetMem, vertexBuff.data(), vertexBuffByteSize);
	targetMem += vertexBuffByteSize;

	std::memcpy(targetMem, indexBuff.data(), indexBuffByteSize);
	targetMem += indexBuffByteSize;

	std::memcpy(targetMem, mats.data(), matsByteSize);
	targetMem += matsByteSize;

	std::memcpy(targetMem, surfaces.data(), surfacesByteSize);
	targetMem += surfacesByteSize;

	std::memcpy(targetMem, textureTable.data(), textureTableByteSize);
	targetMem += textureTableByteSize;

	result.format = EAssetFormat::Mesh;

	modelAsset.Dispose();

	return result;
}
