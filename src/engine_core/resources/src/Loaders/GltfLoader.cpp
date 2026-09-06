#include "GltfLoader.hpp"
#include "Assertions.hpp"
#include "Components/LocalTransform.hpp"
#include "Components/Material3D.hpp"
#include "Components/MeshReference.hpp"
#include "Components/WorldTransform.hpp"
#include "Components/GpuUploadComponent.hpp"
#include "Shared/MaterialOptions.hpp"
#include "crypto/Hashing.hpp"
#include <algorithm>
#include <cstring>
#include <fastgltf/tools.hpp>
#include <glm/ext/vector_float4.hpp>
#include <magic_enum/magic_enum.hpp>
#include <string>
#include <vector>
#include <fstream>
#include <optional>
#include "Logger.hpp"
#include "RHI/IGraphicsDevice.hpp"
#include "Ref.hpp"
#include "ResourceManager.hpp"
#include <fastgltf/types.hpp>
#include "GltfLoadFunctions.hpp"
#include "Scene.hpp"

void Hush::GLTFLoader::AssetHandle::Alloc(std::pmr::memory_resource *pAllocator)
{
	this->allocator = pAllocator;
	this->backing =
		reinterpret_cast<std::byte *>(allocator->allocate(sizeof(fastgltf::Asset), alignof(fastgltf::Asset)));
	// Placement new on the asset handle
	new (this->backing) fastgltf::Asset;
}

void Hush::GLTFLoader::AssetHandle::Dispose()
{
	using namespace fastgltf;
	// Placement new on the asset handle
	auto *asset = reinterpret_cast<fastgltf::Asset *>(this->backing);
	asset->~Asset();
	this->allocator->deallocate(this->backing, sizeof(Asset), alignof(Asset));
	this->backing = nullptr;
}

void Hush::GLTFLoader::FillMeshData(AssetHandle *asset, size_t meshIndex, std::vector<Mesh::Vertex> *outVertexBuffer,
									std::vector<uint32_t> *outIndexBuffer, std::vector<MaterialInfo> *outMaterials,
									std::vector<std::vector<TextureInfo>> *outTexturesByMat,
									std::vector<GeoSurface> *outSurfaces)
{
	HUSH_ASSERT(asset != nullptr, "Unable to fill mesh data with a null asset!");
	auto *gltfAsset = reinterpret_cast<fastgltf::Asset *>(asset->backing);
	fastgltf::Mesh &mesh = gltfAsset->meshes[meshIndex];
	outTexturesByMat->resize(gltfAsset->materials.size());

	for (const fastgltf::Primitive &primitive : mesh.primitives)
	{
		size_t initialVertex = outVertexBuffer->size();
		GeoSurface surfaceToAdd{};
		surfaceToAdd.startIndex = static_cast<uint32_t>(outIndexBuffer->size());
		const fastgltf::Accessor &primitiveIdxAccessor = gltfAsset->accessors[primitive.indicesAccessor.value()];
		surfaceToAdd.count = static_cast<uint32_t>(primitiveIdxAccessor.count);

		fastgltf::iterateAccessor<uint32_t>(*gltfAsset, primitiveIdxAccessor, [&](uint32_t idx) {
			outIndexBuffer->push_back(idx + static_cast<uint32_t>(initialVertex));
		});

		std::vector<glm::vec3> vertexBuffer =
			GltfLoadFunctions::FindAttributeByName<glm::vec3>(primitive, *gltfAsset, "POSITION");
		for (const glm::vec3 &v : vertexBuffer)
		{
			Mesh::Vertex vertexToAdd{};
			vertexToAdd.position = v;
			outVertexBuffer->push_back(vertexToAdd);
		}

		std::vector<glm::vec3> normalBuffer =
			GltfLoadFunctions::FindAttributeByName<glm::vec3>(primitive, *gltfAsset, "NORMAL");
		for (uint32_t i = 0; i < normalBuffer.size(); i++)
		{
			outVertexBuffer->at(i + initialVertex).normal = normalBuffer.at(i);
		}

		// Load the UVs here
		// TODO: LOADUVS()

		std::vector<glm::vec2> texBuffer =
			GltfLoadFunctions::FindAttributeByName<glm::vec2>(primitive, *gltfAsset, "TEXCOORD_0");

		for (uint32_t i = 0; i < texBuffer.size(); i++)
		{
			outVertexBuffer->at(i + initialVertex).uv = {texBuffer.at(i).x, texBuffer.at(i).y};
		}

		// load vertex colors
		std::vector<glm::vec4> colors =
			GltfLoadFunctions::FindAttributeByName<glm::vec4>(primitive, *gltfAsset, "COLOR_0");

		for (uint32_t i = 0; i < colors.size(); i++)
		{
			outVertexBuffer->at(i + initialVertex).color = colors.at(i);
		}

		if (primitive.materialIndex.has_value())
		{
			size_t meshMatIdx = primitive.materialIndex.value();

			// Ref<Graphics::Material3D> materialInstance =
			// MakeMaterial(renderingContext, materialIdx, asset, {}, basePath, meshRef);

			const fastgltf::Material &rawMaterial = gltfAsset->materials[meshMatIdx];
			auto albedo = rawMaterial.pbrData.baseColorFactor;
			const fastgltf::math::nvec3 &emission = rawMaterial.emissiveFactor;
			(void)emission;
			const uint32_t materialResourceId = Hashing::Fnv1a(rawMaterial.name);
			MaterialInfo mat = {
				.pass = GltfLoadFunctions::GetMaterialPassFromFastGltfPass(rawMaterial.alphaMode),
				.resource = materialResourceId,
				.alphaCutoff = rawMaterial.alphaCutoff,
				.albedo = {albedo.x(), albedo.y(), albedo.z(), albedo.w()},
				.emission = {emission.x(), emission.y(), emission.z(), rawMaterial.emissiveStrength}
			};
			std::memcpy(&(mat.name[0]), rawMaterial.name.data(), rawMaterial.name.size());
			outMaterials->push_back(mat);

			// Encode the material as its resource id rather than a raw pointer, so the cooked
			// surface array can be iterated and resolved against the resource manager on load.
			surfaceToAdd.materialResource = materialResourceId;

			// Collect the default PBR texture slots. Each texture is referenced, for now, by its
			// byte offset + size into the original glb file, so the runtime can slice the texture
			// bytes straight out of the file without re-parsing the whole asset.
			// HACK: The PBR binding indices (1..4) are hardcoded here and mirrored in
			// RenderingSystem::OnPreRender and HMeshLoader. This should be driven by the PBR
			// shader reflection instead so the binding numbers are defined in a single place.
			// TODO: Derive the texture bindings from the shader reflection (BindGroupLayoutDescriptor)
			// and propagate them through MaterialInfo / TextureInfo instead of hardcoding them.
			auto collectTexture = [gltfAsset, materialResourceId, &rawMaterial, meshMatIdx,
								   outTexturesByMat](uint32_t binding, std::string_view bindingName,
													 const auto &textureInfoOpt) -> void {
				if (!textureInfoOpt.has_value())
				{
					return;
				}

				const std::vector<fastgltf::Texture> &assetTextures = gltfAsset->textures;
				const fastgltf::Texture &texture = assetTextures.at(textureInfoOpt->textureIndex);
				if (!texture.imageIndex.has_value())
				{
					return;
				}

				size_t imageIdx = texture.imageIndex.value();
				const std::vector<fastgltf::Image> &images = gltfAsset->images;

				const fastgltf::Image &image = images.at(imageIdx);

				TextureInfo texInfo{};
				texInfo.binding = binding;
				// The owning material's resource id, so the cooker can pair each texture back to
				// the material it belongs to without a separate index mapping.
				texInfo.resource = materialResourceId;
				std::memcpy(&(texInfo.name[0]), bindingName.data(), bindingName.size());
				if (!GltfLoadFunctions::GetImageBufferOffsetAndSize(image, *gltfAsset, &texInfo.offset, &texInfo.size))
				{
					LogFormat(ELogLevel::Warn,
							  "Texture '{}' of material '{}' is not embedded in the glb file; skipping", bindingName,
							  std::string_view(rawMaterial.name.data(), rawMaterial.name.size()));
					return;
				}

				// Avoid collecting a slot that a previous primitive sharing this material already filled.
				auto &materialTextures = (*outTexturesByMat)[meshMatIdx];
				for (const TextureInfo &existing : materialTextures)
				{
					if (std::string_view(&(existing.name[0])) == bindingName)
					{
						return;
					}
				}
				materialTextures.push_back(std::move(texInfo));
			};

			collectTexture(1, "albedo", rawMaterial.pbrData.baseColorTexture);
			collectTexture(2, "metalRough", rawMaterial.pbrData.metallicRoughnessTexture);
			collectTexture(3, "normal", rawMaterial.normalTexture);
			collectTexture(4, "emissive", rawMaterial.emissiveTexture);
		}

		// Correct normals if empty
		if (normalBuffer.empty())
		{
			LogWarn("Normals empty and not generated!");
			// innerMeshRef->CalculateNormals();
		}

		outSurfaces->push_back(surfaceToAdd);
	}
}

Hush::GltfLoadFunctions::EError Hush::GLTFLoader::LoadAssetFromBinary(std::span<const std::byte> data,
																	  Hush::GLTFLoader::AssetHandle *outAsset)
{
	HUSH_ASSERT(outAsset != nullptr, "Can't load asset into null handle!");
	auto res = Hush::GltfLoadFunctions::GetAssetFromBinary(data);
	if (!res)
	{
		return Hush::GltfLoadFunctions::EError::InvalidMeshFile;
	}
	auto *assetInPlace = reinterpret_cast<fastgltf::Asset *>(outAsset->backing);
	*assetInPlace = std::move(res.get());

	return Hush::GltfLoadFunctions::EError::None;
}

void Hush::GLTFLoader::ProcessPrimitives(const RenderingContext &renderingContext, const fastgltf::Asset &asset,
										 const fastgltf::Mesh &mesh, const std::filesystem::path &basePath,
										 Ref<Mesh> &innerMeshRef, MeshReference *meshRef)
{
#ifdef DEBUG
	if (meshRef != nullptr)
	{
		HUSH_ASSERT(meshRef->GetMesh().Get() == innerMeshRef.Get(),
					"Mesh and inner mesh should point to the same resource!");
	}
#endif
	std::vector<uint32_t> &indexRef = innerMeshRef->GetIndexBuffer();
	std::vector<Mesh::Vertex> &vertexRef = innerMeshRef->GetVertexBuffer();
	indexRef.clear();
	vertexRef.clear();

	// The mesh is shared (ref-counted) across every MeshReference that points
	// at it, so re-importing the same glTF appends a duplicate surface on each
	// drop unless we clear the list first. Without this, the second drop of the
	// same model doubles the surfaces and the draw list, which (combined with a
	// cache that used to rebuild per-surface) leaked a freed bind-group pointer
	// into the first draw.
	innerMeshRef->GetSurfaces().clear();

	for (const fastgltf::Primitive &primitive : mesh.primitives)
	{
		size_t initialVertex = vertexRef.size();
		GeoSurface surfaceToAdd{};
		surfaceToAdd.startIndex = static_cast<uint32_t>(indexRef.size());
		const fastgltf::Accessor &primitiveIdxAccessor = asset.accessors[primitive.indicesAccessor.value()];
		surfaceToAdd.count = static_cast<uint32_t>(primitiveIdxAccessor.count);

		indexRef.reserve(indexRef.size() + primitiveIdxAccessor.count);

		fastgltf::iterateAccessor<uint32_t>(asset, primitiveIdxAccessor, [&](uint32_t idx) {
			indexRef.push_back(idx + static_cast<uint32_t>(initialVertex));
		});

		std::vector<glm::vec3> vertexBuffer =
			GltfLoadFunctions::FindAttributeByName<glm::vec3>(primitive, asset, "POSITION");
		for (const glm::vec3 &v : vertexBuffer)
		{
			Mesh::Vertex vertexToAdd{};
			vertexToAdd.position = v;
			vertexRef.push_back(vertexToAdd);
		}

		std::vector<glm::vec3> normalBuffer =
			GltfLoadFunctions::FindAttributeByName<glm::vec3>(primitive, asset, "NORMAL");
		for (uint32_t i = 0; i < normalBuffer.size(); i++)
		{
			vertexRef.at(i + initialVertex).normal = normalBuffer.at(i);
		}

		// Load the UVs here
		// TODO: LOADUVS()

		std::vector<glm::vec2> texBuffer =
			GltfLoadFunctions::FindAttributeByName<glm::vec2>(primitive, asset, "TEXCOORD_0");

		for (uint32_t i = 0; i < texBuffer.size(); i++)
		{
			vertexRef.at(i + initialVertex).uv = {texBuffer.at(i).x, texBuffer.at(i).y};
		}

		// load vertex colors
		std::vector<glm::vec4> colors = GltfLoadFunctions::FindAttributeByName<glm::vec4>(primitive, asset, "COLOR_0");

		for (uint32_t i = 0; i < colors.size(); i++)
		{
			vertexRef.at(i + initialVertex).color = colors.at(i);
		}

		if (primitive.materialIndex.has_value())
		{
			size_t materialIdx = primitive.materialIndex.value();

			Ref<Graphics::Material3D> materialInstance =
				MakeMaterial(renderingContext, materialIdx, asset, {}, basePath, meshRef);
			// Keep alive on the Mesh component
			if (meshRef != nullptr)
			{
				meshRef->PushMaterial(materialInstance);
			}

			// Non-owning ref on the surface
			surfaceToAdd.materialResource = materialInstance.GetResourceId();
		}

		// Correct normals if empty
		if (normalBuffer.empty())
		{
			innerMeshRef->CalculateNormals();
		}

		innerMeshRef->AddSurface(std::move(surfaceToAdd));
	}
}

/// @brief This is a temporary function, we need to move this behavior to HushCooker, but this will work to prove we can
/// already load and render objects
Hush::Entity Hush::GLTFLoader::GenerateMeshEntities(const RenderingContext &renderingContext,
													const std::string_view &virtualPath)
{
	auto hostPathRes = renderingContext.virtualFilesystem->ResolveHostPath(virtualPath);
	HUSH_RESULT_ASSERT(hostPathRes, "Could not resolve virtual path of meshes!");
	// Open the file and parse it with the gltf loader functions
	auto assetRes = GltfLoadFunctions::GetAssetFromFile(hostPathRes.value());
	HUSH_COND_FAIL_MSG_V(assetRes, Entity::Null(), "Could not load mesh at {}, error: {}", virtualPath,
						 magic_enum::enum_name(assetRes.error()));
	std::vector<Entity> entities;

	// For each node, generate an entity with a transform
	Scene *activeScene = renderingContext.activeScene;
	ResourceManager *resourceManager = renderingContext.resourceManager;

	int32_t generatedEntities = 0;
	for (const fastgltf::Mesh &mesh : assetRes->meshes)
	{
		Entity entity = activeScene->CreateEntityWithName(mesh.name.empty() ? "LoadedMesh" : mesh.name);
		entity.AddComponent<WorldTransform>();
		entity.AddComponent<LocalTransform>();
		// Create the mesh
		Ref<Mesh> meshRef = resourceManager->AllocateRef<Mesh>(std::string(virtualPath) + std::string(mesh.name));
		auto &meshComponent = entity.EmplaceComponent<MeshReference>(meshRef);

		meshComponent.SetResourcePath(virtualPath, mesh.name);
		meshRef->SetName(mesh.name);

		std::filesystem::path basePath = hostPathRes.value().parent_path();
		ProcessPrimitives(renderingContext, assetRes.get(), mesh, basePath, meshRef, &meshComponent);

		// Generate the material per primitive here

		meshRef->CalculateTangentBasis();

		// Load everything into Mesh components
		// Add a GpuUploadComponent for the UploadResourceSystem to pick it up
		entity.AddComponent<Renderer::GpuUploadComponent>();

		entities.emplace_back(std::move(entity));
		generatedEntities++;
	}

	Entity fatherEntity = Entity::Null();

	HUSH_COND_FAIL_MSG_V(generatedEntities > 0, fatherEntity, "Could not generate meshes for an empty GLTF model");

	// Create at origin, this should potentially be at the mouse's world position later on
	if (generatedEntities > 1)
	{
		fatherEntity = activeScene->CreateEntityWithName(hostPathRes.value().stem().string());
		fatherEntity.AddComponent<WorldTransform>();
		fatherEntity.AddComponent<LocalTransform>();
	}

	for (fastgltf::Node &node : assetRes->nodes)
	{
		if (!node.meshIndex.has_value())
		{
			continue;
		}

		Entity &entity = entities[node.meshIndex.value()];
		LocalTransform *localXformComponent = entity.GetComponent<LocalTransform>();

		glm::mat4 nodeXform = GltfLoadFunctions::GetNodeTransform(node);

		localXformComponent->SetTransformationMatrix(nodeXform);
		if (fatherEntity.IsValid())
		{
			fatherEntity.AddChild(entity);
		}

		if (node.children.empty())
		{
			continue;
		}
		for (size_t &c : node.children)
		{
			Entity &childEntity = entities[c];
			entity.AddChild(childEntity);
		}
	}

	// Finished entity uploading
	if (fatherEntity.IsValid())
	{
		return fatherEntity;
	}
	return std::move(entities[0]);
}

bool Hush::GLTFLoader::LoadMeshes(const RenderingContext &renderingContext, const std::string_view &virtualPath)
{
	auto hostPathRes = renderingContext.virtualFilesystem->ResolveHostPath(virtualPath);
	HUSH_RESULT_ASSERT(hostPathRes, "Could not resolve virtual path of meshes!");
	// Open the file and parse it with the gltf loader functions
	auto assetRes = GltfLoadFunctions::GetAssetFromFile(hostPathRes.value());
	HUSH_COND_FAIL_MSG_V(assetRes, false, "Could not load mesh at {}, error: {}", virtualPath,
						 magic_enum::enum_name(assetRes.error()));

	ResourceManager *resourceManager = renderingContext.resourceManager;

	for (const fastgltf::Mesh &mesh : assetRes->meshes)
	{
		// NOTE: Create the mesh, this will live until the deletion of the frame if unclaimed
		Ref<Mesh> meshRef = resourceManager->AllocateRef<Mesh>(std::string(virtualPath) + std::string(mesh.name));

		// Rudamentary check to see if the mesh was loaded before
		if (!meshRef->GetIndexBuffer().empty())
		{
			return true;
		}

		meshRef->SetName(mesh.name);
		std::filesystem::path basePath = hostPathRes.value().parent_path();
		ProcessPrimitives(renderingContext, assetRes.get(), mesh, basePath, meshRef);

		// Generate the material per primitive here

		meshRef->CalculateTangentBasis();

		// Load everything into Mesh components
		// Add a GpuUploadComponent for the UploadResourceSystem to pick it up
		// entity.AddComponent<Renderer::GpuUploadComponent>();

		// entities.emplace_back(std::move(entity));
		// generatedEntities++;
	}
	return true;
}

namespace
{

	std::optional<std::vector<std::byte>> LoadGlTfImageData(const fastgltf::Asset &asset, const fastgltf::Image &image,
															const std::filesystem::path &basePath)
	{
		fastgltf::MimeType mimeType = fastgltf::MimeType::None;
		std::span<const std::byte> span = Hush::GltfLoadFunctions::ExtractImageBuffer(image, asset, &mimeType);
		if (!span.empty())
		{
			return std::vector<std::byte>(span.begin(), span.end());
		}

		const auto *uriData = std::get_if<fastgltf::sources::URI>(&image.data);
		if (uriData != nullptr)
		{
			std::filesystem::path imagePath = basePath / uriData->uri.fspath();
			std::ifstream file(imagePath, std::ios::binary | std::ios::ate);
			if (!file)
			{
				return {};
			}
			auto size = file.tellg();
			std::vector<std::byte> buf(static_cast<size_t>(size));
			file.seekg(0);
			file.read(reinterpret_cast<char *>(buf.data()), static_cast<std::streamsize>(size));
			return buf;
		}

		return {};
	}

} // namespace

Hush::Ref<Hush::Graphics::Material3D> Hush::GLTFLoader::MakeMaterial(
	const RenderingContext &renderingContext, size_t materialIdx, const fastgltf::Asset &asset,
	const std::vector<Graphics::IGraphicsTexture *> &loadedTextures, const std::filesystem::path &basePath,
	MeshReference *meshRefComp)
{
	(void)loadedTextures;
	const fastgltf::Material &material = asset.materials.at(materialIdx);
	EMaterialPass passType = GltfLoadFunctions::GetMaterialPassFromFastGltfPass(material.alphaMode);

	ResourceManager *resourceManager = renderingContext.resourceManager;
	const Graphics::Material3DDescriptor *defaultMaterialDesc = renderingContext.materialDescriptor;
	Graphics::IGraphicsDevice *graphicsDevice = renderingContext.device;

	auto materialInstance = resourceManager->AllocateRef<Graphics::Material3D>(material.name);
	materialInstance->SetMaterialPass(passType);
	if (!materialInstance->IsInitialized())
	{
		Graphics::Material3D::EError err = materialInstance->Init(graphicsDevice, *defaultMaterialDesc);
		HUSH_COND_FAIL_MSG_V(err == Graphics::Material3D::EError::None, {}, "Unable to initialize material: {}",
							 magic_enum::enum_name(err));
	}

	// Albedo
	materialInstance->SetProperty("colorFactors",
								  *reinterpret_cast<const glm::vec4 *>(&material.pbrData.baseColorFactor));
	materialInstance->SetProperty("emissionFactors", glm::vec4(material.emissiveFactor.x(), material.emissiveFactor.y(),
															   material.emissiveFactor.z(), material.emissiveStrength));
	materialInstance->SetProperty("alphaCutoff", material.alphaCutoff);
	constexpr uint32_t useNormalsFlag = 1;
	materialInstance->SetProperty("optionFlags", useNormalsFlag);
	materialInstance->SetName(material.name);
	materialInstance->SetAlphaBlendMode(EAlphaBlendMode::OneMinusSrcAlpha);


	// glTF PBR bindings: 1 = baseColor, 2 = metallicRoughness,
	//                     3 = normal, 4 = emissive
	constexpr uint32_t kBindingAlbedo = 1;
	constexpr uint32_t kBindingMetalRough = 2;
	constexpr uint32_t kBindingNormal = 3;
	constexpr uint32_t kBindingEmissive = 4;

	auto loadMaterialTexture = [&](uint32_t binding, const auto &textureInfo, const char *texName) -> void {
		if (!textureInfo.has_value())
		{
			return;
		}

		size_t textureIdx = textureInfo->textureIndex;
		const auto &gltfTexture = asset.textures.at(textureIdx);
		if (!gltfTexture.imageIndex.has_value())
		{
			return;
		}

		size_t imageIdx = gltfTexture.imageIndex.value();
		const fastgltf::Image &image = asset.images.at(imageIdx);

		std::string texUniqueName = std::string(material.name) + "_" + texName;

		// Check if already loaded for this material in the mesh reference
		auto &texRefs = meshRefComp->GetMaterialTextureRefs()[materialInstance.Get()];
		auto existingIt = texRefs.find(binding);
		if (existingIt != texRefs.end())
		{
			auto *gpuTex = existingIt->second->GetGpuTexture();
			materialInstance->SetTexture(binding, gpuTex);
			return;
		}

		auto imgData = LoadGlTfImageData(asset, image, basePath);
		if (!imgData.has_value())
		{
			return;
		}

		auto result = resourceManager->LoadTextureFromData(texUniqueName, *imgData);
		if (result.has_error())
		{
			return;
		}

		Ref<TextureComponent> texRef = std::move(result.value());
		auto *gpuTex = texRef->GetGpuTexture(); // May be null (async upload)
		materialInstance->SetTexture(binding, gpuTex);
		texRefs[binding] = std::move(texRef);
	};

	loadMaterialTexture(kBindingAlbedo, material.pbrData.baseColorTexture, "albedo");
	loadMaterialTexture(kBindingMetalRough, material.pbrData.metallicRoughnessTexture, "metalRough");
	loadMaterialTexture(kBindingNormal, material.normalTexture, "normal");
	loadMaterialTexture(kBindingEmissive, material.emissiveTexture, "emissive");

	materialInstance->FlushProperties(renderingContext.device);
	return materialInstance;
}
