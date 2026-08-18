#include "HMeshLoader.hpp"
#include "Assertions.hpp"
#include "Components/Material3D.hpp"
#include "Components/MeshReference.hpp"
#include "HAsset.hpp"
#include "Loaders/CrossLoaderDefinitions.hpp"
#include "ResourceManager.hpp"
#include "Shared/MaterialPass.hpp"
#include "Shared/Mesh.hpp"
#include <cstddef>
#include <cstdint>
#include <string_view>

using namespace Hush;

// Intentionally keeping these structs only in this compilation unit, they are supposed to mirror the Mesh Cooker's, but
// we do not want to have a dependency to that module here

struct HMeshHeader
{
	uint32_t vertexCount;
	uint32_t indexCount;
	uint32_t surfaceCount;
	uint32_t materialCount;
};

struct HMeshMaterialInfo
{
	// 63 + null
	static constexpr size_t MAX_MAT_NAME = 64;
	EMaterialPass pass;
	uint32_t resource;
	float alphaCutoff;
	glm::vec4 albedo;
	char name[MAX_MAT_NAME];
};

bool HMeshLoader::LoadMeshFromBinary(std::span<const std::byte> data, MeshReference *outRef,
									 const RenderingContext* renderingCtx)
{
	HUSH_ASSERT(outRef != nullptr, "Cannot load a mesh into a null mesh reference component!");
	std::optional<HAsset> asset = HAsset::Read(data);
	if (!asset.has_value())
	{
		return false;
	}
	ResourceManager* resourceManager = renderingCtx->resourceManager;

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
			resourceManager->GetRefOrNull<Graphics::Material3D>(materialInfo->resource);
		if (!existingMat.IsNull())
		{
			// Push the material 
			outRef->PushMaterial(existingMat);
			continue;
		}
		// Otherwise, create it
		auto matName = std::string_view(&materialInfo->name[0]);
		existingMat = resourceManager->AllocateRefKnwonID<Graphics::Material3D>(materialInfo->resource);
		existingMat->Init(renderingCtx->device, *renderingCtx->materialDescriptor);
		existingMat->SetMaterialPass(materialInfo->pass);
		// TODO: Make these reflect the .hshader metadata
		existingMat->SetProperty("colorFactors", materialInfo->albedo);
		existingMat->SetProperty("alphaCutoff", materialInfo->alphaCutoff);
		existingMat->SetName(matName);
		outRef->PushMaterial(existingMat);
	}
	const size_t matDataSize = sizeof(HMeshMaterialInfo) * header->materialCount;
	rawData += matDataSize;

	const auto* rawSurfaces = reinterpret_cast<const GeoSurface*>(rawData);
	std::vector<GeoSurface> &matSurfaces = innerMesh->GetSurfaces();
	matSurfaces.resize(header->surfaceCount);
	std::memcpy(matSurfaces.data(), rawSurfaces, sizeof(GeoSurface) * header->surfaceCount);

	// That should be it c:
	return true;
}
