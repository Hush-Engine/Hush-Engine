#include "MeshCooker.hpp"
#include "AssetFormat.hpp"
#include "Components/MeshReference.hpp"
#include "HMeta.hpp"
#include "Loaders/GltfLoadFunctions.hpp"
#include "Loaders/GltfLoader.hpp"
#include "ICooker.hpp"
#include "Logger.hpp"
#include "crypto/Hashing.hpp"
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
	result.id = Hashing::Fnv1a64(srcVPath);
	// MAYBE: Perhaps in the future we might want compression for large meshes?
	result.compression = ECompressionFormat::None;
	result.outputFormat = EAssetFormat::Mesh;
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
	GltfLoadFunctions::EError err = GLTFLoader::LoadAssetFromBinary(input, &modelAsset);
	if (err != GltfLoadFunctions::EError::None)
	{
		LogFormat(ELogLevel::Error, "Could not process file {}, internal error: {}", ctx.sourceVPath,
				  magic_enum::enum_name(err));
		return ECookError::InvalidData;
	}

	// With the asset loaded, we need to write its VertexBuffer, IndexBuffer and Materials
	std::vector<Mesh::Vertex> vertexBuff;
	std::vector<uint32_t> indexBuff;
	std::vector<GLTFLoader::MaterialInfo> mats;
	std::vector<std::vector<GLTFLoader::TextureInfo>> texturesByMaterial;

	GLTFLoader::FillMeshData(&modelAsset, 0, &vertexBuff, &indexBuff, &mats, &texturesByMaterial);

	// For now we'll do just one

	// Cook it into the asset
	CookResult result{};
	std::vector<std::byte> *payload = &result.payload;
	// We need a header with some metadata
	Header header{};
	header.vertexCount = static_cast<uint32_t>(vertexBuff.size());
	header.indexCount = static_cast<uint32_t>(indexBuff.size());
	header.materialCount = static_cast<uint32_t>(mats.size());

	const size_t vertexBuffByteSize = (sizeof(Mesh::Vertex) * vertexBuff.size());
	const size_t indexBuffByteSize = (sizeof(uint32_t) * indexBuff.size());
	const size_t matsByteSize = (sizeof(GLTFLoader::MaterialInfo) * mats.size());
	const size_t fileSize = sizeof(Header) + vertexBuffByteSize + indexBuffByteSize + matsByteSize;

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

	result.format = EAssetFormat::Mesh;

	return result;
}
