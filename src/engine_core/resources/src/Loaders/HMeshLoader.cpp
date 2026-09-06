#include "HMeshLoader.hpp"
#include "Assertions.hpp"
#include "Components/Material3D.hpp"
#include "Components/MeshReference.hpp"
#include "Components/TextureComponent.hpp"
#include "HAsset.hpp"
#include "Logger.hpp"
#include "ResourceManager.hpp"
#include "Shared/MaterialOptions.hpp"
#include "Shared/MaterialPass.hpp"
#include "Shared/Mesh.hpp"
#include "VirtualFilesystem.hpp"
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <fstream>
#include <ios>
#include <string>
#include <string_view>
#include <vector>

using namespace Hush;

// Intentionally keeping these structs only in this compilation unit, they are supposed to mirror the Mesh Cooker's, but
// we do not want to have a dependency to that module here

struct HMeshHeader
{
	uint32_t vertexCount;
	uint32_t indexCount;
	uint32_t surfaceCount;
	uint32_t materialCount;
	uint32_t textureCount;
	/// @brief Virtual path of the original source asset (e.g. a glb) the textures are sliced from
	char sourcePath[256];
};

struct HMeshMaterialInfo
{
	// 63 + null
	static constexpr size_t MAX_MAT_NAME = 64;
	EMaterialPass pass;
	uint32_t resource;
	float alphaCutoff;
	glm::vec4 albedo;
	glm::vec4 emission; // w for intensity
	char name[MAX_MAT_NAME];
};

struct HMeshTextureInfo
{
	static constexpr size_t MAX_TEX_NAME = 64;
	uint32_t materialResource;
	uint32_t binding;
	/// @brief File-relative byte offset into the original source asset
	uint64_t offset;
	uint64_t size;
	char name[MAX_TEX_NAME];
};

bool HMeshLoader::LoadMeshFromBinary(std::span<const std::byte> data, MeshReference *outRef,
									 const RenderingContext *renderingCtx)
{
	HUSH_ASSERT(outRef != nullptr, "Cannot load a mesh into a null mesh reference component!");
	std::optional<HAsset> asset = HAsset::Read(data);
	if (!asset.has_value())
	{
		return false;
	}
	ResourceManager *resourceManager = renderingCtx->resourceManager;

	const std::vector<std::byte> &modelData = asset.value().payload;
	const std::byte *rawData = modelData.data();

	Ref<Mesh> &innerMesh = outRef->GetMesh();
	const auto *header = reinterpret_cast<const HMeshHeader *>(rawData);
	rawData += sizeof(HMeshHeader);

	std::vector<Mesh::Vertex> &vb = innerMesh->GetVertexBuffer();
	std::vector<uint32_t> &ib = innerMesh->GetIndexBuffer();

	vb.resize(header->vertexCount);
	ib.resize(header->indexCount);

	const size_t vertexDataSize = sizeof(Mesh::Vertex) * header->vertexCount;
	std::memcpy(vb.data(), rawData, vertexDataSize);
	rawData += vertexDataSize;

	const size_t indexDataSize = sizeof(uint32_t) * header->indexCount;
	std::memcpy(ib.data(), rawData, indexDataSize);
	rawData += indexDataSize;

	const auto *materialInfo = reinterpret_cast<const HMeshMaterialInfo *>(rawData);
	for (uint32_t i = 0; i < header->materialCount; i++)
	{
		// If a material info has a resource id and it exists in the disk resources, we can load it directly.
		// Otherwise we get/create a new one.
		Ref<Graphics::Material3D> existingMat =
			resourceManager->GetRefOrNull<Graphics::Material3D>(materialInfo[i].resource);
		if (!existingMat.IsNull())
		{
			// Push the material
			outRef->PushMaterial(existingMat);
			continue;
		}
		// Otherwise, create it
		auto matName = std::string_view(&(materialInfo[i].name[0]));
		existingMat = resourceManager->AllocateRefKnwonID<Graphics::Material3D>(materialInfo[i].resource);
		existingMat->SetMaterialPass(materialInfo[i].pass);
		existingMat->SetAlphaBlendMode(EAlphaBlendMode::OneMinusSrcAlpha);
		// TODO: Make these reflect the .hshader metadata
		existingMat->Init(renderingCtx->device, *renderingCtx->materialDescriptor);
		existingMat->SetProperty("colorFactors", materialInfo[i].albedo);
		existingMat->SetProperty("emissionFactors", materialInfo[i].emission);
		existingMat->SetProperty("alphaCutoff", materialInfo[i].alphaCutoff);
		existingMat->SetName(matName);
		outRef->PushMaterial(existingMat);
	}
	const size_t matDataSize = sizeof(HMeshMaterialInfo) * header->materialCount;
	rawData += matDataSize;

	const auto *rawSurfaces = reinterpret_cast<const GeoSurface *>(rawData);
	std::vector<GeoSurface> &matSurfaces = innerMesh->GetSurfaces();
	matSurfaces.resize(header->surfaceCount);
	std::memcpy(matSurfaces.data(), rawSurfaces, sizeof(GeoSurface) * header->surfaceCount);
	rawData += sizeof(GeoSurface) * header->surfaceCount;

	// Texture table: each entry references a byte range in the original source asset (file-relative),
	// so we seek + read it directly without caring about the source container format.
	const auto *textureInfo = reinterpret_cast<const HMeshTextureInfo *>(rawData);
	if (header->textureCount > 0)
	{
		const auto consumed = static_cast<size_t>(rawData - modelData.data());
		if (consumed > modelData.size())
		{
			return false;
		}
		const size_t remainingBytes = modelData.size() - consumed;
		const size_t textureTableBytes = sizeof(HMeshTextureInfo) * header->textureCount;
		if (textureTableBytes > remainingBytes)
		{
			// The mesh was almost certainly cooked before the texture table / sourcePath fields were
			// added to the header, so the count reads garbage from what used to be vertex data.
			// Re-cooking the asset upgrades it to the current mesh format.
			LogFormat(ELogLevel::Error,
					  "HMeshLoader: texture table of {} entries does not fit the payload ({} bytes remaining); "
					  "the asset was cooked with an older mesh format, re-cook it",
					  header->textureCount, remainingBytes);
			return false;
		}

		const std::string_view sourcePath(header->sourcePath);
		if (sourcePath.empty())
		{
			LogFormat(ELogLevel::Error, "HMeshLoader: mesh has textures but no source path in the header");
			return false;
		}

		// CHECK THIS FILESYSTEM THING
		auto resolveRes = renderingCtx->virtualFilesystem->ResolveHostPath(sourcePath);
		if (resolveRes.has_error())
		{

			LogFormat(ELogLevel::Error, "HMeshLoader: could not resolve source asset '{}' for texture slicing",
					  sourcePath);

			return false;
		}
		std::ifstream inputFile;
		inputFile.open(resolveRes.value(), std::ios_base::binary | std::ios_base::in);

		if (!inputFile.is_open())
		{
			LogFormat(ELogLevel::Error, "HMeshLoader: could not open source asset '{}' for texture slicing",
					  sourcePath);
			return false;
		}

		for (uint32_t i = 0; i < header->textureCount; i++)
		{
			const HMeshTextureInfo &tex = textureInfo[i];
			Ref<Graphics::Material3D> mat = resourceManager->GetRefOrNull<Graphics::Material3D>(tex.materialResource);
			if (mat.IsNull())
			{
				continue;
			}

			std::vector<std::byte> texData{};
			texData.resize(tex.size);

			inputFile.seekg(static_cast<int64_t>(tex.offset));
			inputFile.read(reinterpret_cast<char *>(texData.data()), static_cast<int64_t>(tex.size));

			// Dedup key must match the GLTF direct path (material name + "_" + binding name) so a
			// re-cooked mesh shares the same TextureComponent as the original load.
			std::string texUniqueName = std::string(mat->GetName()) + std::string("_") + std::string(&(tex.name[0]));
			auto loadRes = resourceManager->LoadTextureFromData(texUniqueName, {texData.data(), tex.size});
			if (loadRes.has_error())
			{
				continue;
			}
			Ref<TextureComponent> texRef = std::move(loadRes.value());

			outRef->GetMaterialTextureRefs()[mat.Get()][tex.binding] = texRef;
			mat->SetTexture(tex.binding, texRef->GetGpuTexture());
		}
	}

	// That should be it c:
	return true;
}
