#include "Assertions.hpp"
#include "Components/LocalTransform.hpp"
#include "Components/WorldTransform.hpp"
#include "Entity.hpp"
#include "IFile.hpp"
#include "Loaders/IModelLoader.hpp"
#include "Ref.hpp"
#include "Renderer.hpp"
#include "Result.hpp"
#include "Shared/IMaterial3D.hpp"
#include "VirtualFilesystem.hpp"
#include <array>
#include <cstdint>
#include <memory>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>
#define VK_NO_PROTOTYPES
#include <glm/ext/matrix_float4x4.hpp>
#include "VulkanLoader.hpp"
#include "Vulkan/VulkanRenderer.hpp"
#include "Shared/Mesh.hpp"
#include "Shared/Types/ImageExtent3D.hpp"
#include "Vulkan/GltfMetallicRoughness.hpp"
#include "Vulkan/VkDescriptors.hpp"
#include <SDL_render.h>
#include <glm/ext/vector_float3.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/ext/vector_int3.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/core.hpp>
#include "Shared/ImageTexture.hpp"
#include "Loaders/GltfLoadFunctions.hpp"
#include "Components/MeshReference.hpp"
#include "../../core/src/Scene.hpp"


void Hush::VulkanLoader::SetResourceManager(ResourceManager* resourceManager, VirtualFilesystem* filesystem) {
	this->m_resourceManager = resourceManager;
	this->m_filesystem = filesystem;
}

Hush::ResourceManager *Hush::VulkanLoader::GetResourceManager() const {
	return this->m_resourceManager;
}


Hush::Result<std::vector<Hush::Entity>, Hush::IModelLoader::EError> Hush::VulkanLoader::LoadMeshes(IRenderer *engine, const std::filesystem::path& filePath, Scene *activeScene)
{
	HUSH_ASSERT(this->m_resourceManager != nullptr, "Failed to load meshes resource manager is null!");
	fastgltf::Expected<fastgltf::Asset> loadedAsset = GltfLoadFunctions::GetAssetFromFile(filePath);
	
	HUSH_ASSERT(loadedAsset, "GLTF asset at {} not properly loaded, error: {}!", filePath.string(),
				fastgltf::getErrorMessage(loadedAsset.error()));

	std::vector<Entity> entities;
	// TODO: render these meshes instead of the loaded nodes, or store these in there idk
	// HUSH_ASSERT(loadedAsset->meshes.size() != loadedAsset->nodes.size(), "Meshes vector size does not match nodes
	// size");
	
	std::array<char, Entity::MAX_ENTITY_NAME_LENGTH> nameBuffer{};
	for (const fastgltf::Mesh &mesh : loadedAsset->meshes)
	{
		if (mesh.name.empty()) {
			constexpr std::string_view defaultName = "LoadedMesh";
			strcpy_s(nameBuffer.data(), nameBuffer.size(), defaultName.data());
		}
		else {
			strcpy_s(nameBuffer.data(), nameBuffer.size(), mesh.name.c_str());
		}
		Entity entity = activeScene->CreateEntityWithName(nameBuffer.data());
		entity.AddComponent<WorldTransform>();
		entity.AddComponent<LocalTransform>();
		// This also adds the component to the entity
		// TODO: We should probably change this so that it returns void and we add the component a line before calling
		// the function
		this->CreateMeshFromGltfMesh(mesh, loadedAsset.get(), entity, engine, filePath);
		entities.emplace_back(std::move(entity));
	}

	for (fastgltf::Node &node : loadedAsset->nodes)
	{
		if (!node.meshIndex.has_value())
		{
			continue;
		}

		Entity &entity = entities[node.meshIndex.value()];
		WorldTransform *xformComponent = entity.GetComponent<WorldTransform>();
		LocalTransform *localXformComponent = entity.GetComponent<LocalTransform>();

		glm::mat4 nodeXform = GltfLoadFunctions::GetNodeTransform(node);

		xformComponent->SetTransformationMatrix(nodeXform);
		localXformComponent->SetTransformationMatrix(nodeXform);

		if (node.children.empty())
		{
			continue;
		}
		for (size_t &c : node.children)
		{
			// Foreach node, we need to adjust the transformation so that the world xform is the world * local
			Entity &childEntity = entities[c];
			// At this point world and local transforms are the same, so we can just fetch the world and apply the
			// parent transformation to it
			WorldTransform *childXform = childEntity.GetComponent<WorldTransform>();
			LocalTransform *childlocalXform = childEntity.GetComponent<LocalTransform>();
			childXform->SetTransformationMatrix(xformComponent->XForm(*childXform));
			childlocalXform->SetParent(entity.GetId());
		}
	}

	return entities;
}

// TODO: This is now completely renderer agnostic
Hush::GpuAllocatedImage Hush::VulkanLoader::LoadTexture(Hush::IRenderer *engine, const ImageTexture &texture)
{
	ImageExtent3D extent{static_cast<uint32_t>(texture.GetWidth()), static_cast<uint32_t>(texture.GetHeight()), 1};

	constexpr Color::EFormat defaultImageFormat = Color::EFormat::RGBA8Unorm;

	return engine->CreateImage(texture.GetImageData(), extent, defaultImageFormat, VK_IMAGE_USAGE_SAMPLED_BIT);
}

std::vector<Hush::GpuAllocatedImage> Hush::VulkanLoader::LoadAllTextures(const fastgltf::Asset &asset,
																		 IRenderer *engine, const std::filesystem::path& filePath)
{
	std::vector<GpuAllocatedImage> loadedTexturesResult;
	loadedTexturesResult.reserve(asset.images.size());
	std::string textQuery;
	for (size_t i = 0; i < asset.images.size(); i++)
	{
		const fastgltf::Image& image = asset.images[i];
		textQuery = filePath.stem().string()
			.append("_")
			.append(std::to_string(i));
		
		// Check if anything matches the textQuery
		Result<FileInfo, IFile::EError> textureFileRes = this->m_filesystem->GetFirstMatchingSubstr(filePath.parent_path(), textQuery);
		
		Ref<ImageTexture> texture{};
		if (textureFileRes.has_error()) {
			// The mesh doesn't have a pre-generated texture, so we can safely fallback to the gltf data source
			texture = GltfLoadFunctions::TextureFromImageDataSource(asset, image, textQuery, this->m_resourceManager);
		}
		else {
			// We use the pre-generated file, mostly bc it will already have calculated mipmaps and all that fancy stuff
			texture = this->m_resourceManager->LoadTexture(textureFileRes.value().path.string());
		}
		
		GpuAllocatedImage loadedImage = LoadTexture(engine, *texture.Get());
		loadedTexturesResult.emplace_back(loadedImage);
	}
	return loadedTexturesResult;
}

Hush::MeshReference *Hush::VulkanLoader::CreateMeshFromGltfMesh(const fastgltf::Mesh &mesh, const fastgltf::Asset &asset,
													   Entity &entityRef, IRenderer *engine, const std::filesystem::path& filePath)
{
	auto* rendererImpl = dynamic_cast<VulkanRenderer*>(engine);
	// Load a mesh through the resource loader
	Ref<Mesh> meshAsset = this->m_resourceManager->AllocateRef<Mesh>(mesh.name); // TODO: We should probably append the name of the file or something to avoid conflicts

	meshAsset->SetName(mesh.name);

	// Clear out the vector buffers
	std::vector<uint32_t> &indexRef = meshAsset->GetIndexBuffer();
	std::vector<Mesh::Vertex> &vertexRef = meshAsset->GetVertexBuffer();
	indexRef.clear();
	vertexRef.clear();

	// TODO: constexpr(?
	const std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
		{VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3},
		{VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3},
		{VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1}};

	DescriptorAllocatorGrowable descriptorPool;
	descriptorPool.Init(volkGetLoadedDevice(), static_cast<uint32_t>(asset.materials.size()), sizes);

	std::vector<GpuAllocatedImage> loadedTextures = this->LoadAllTextures(asset, rendererImpl, filePath);
	std::unordered_map<std::uintptr_t, std::shared_ptr<IMaterial3D>> loadedMaterials;

	for (const fastgltf::Primitive &primitive : mesh.primitives)
	{
		size_t initialVertex = vertexRef.size();
		GeoSurface surfaceToAdd{};
		surfaceToAdd.startIndex = static_cast<uint32_t>(indexRef.size());
		const fastgltf::Accessor &primitiveIdxAccessor = asset.accessors[primitive.indicesAccessor.value()];
		surfaceToAdd.count = static_cast<uint32_t>(primitiveIdxAccessor.count);

		// TODO: Make a function that can load any arbitrary primitive from glTF

		// load indexes
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

		// load UVs
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
			std::shared_ptr<IMaterial3D> materialInstance =
				GenerateMaterial(materialIdx, asset, rendererImpl, descriptorPool, loadedTextures);
			surfaceToAdd.material = materialInstance;
		}

		// Correct normals if empty
		if (normalBuffer.empty())
		{
			meshAsset->CalculateNormals();
		}

		meshAsset->AddSurface(std::move(surfaceToAdd));
	}

	std::vector<VkSampler> samplers;

	for (const fastgltf::Sampler &sampler : asset.samplers)
	{
		VkSamplerCreateInfo sampl = {.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr};
		sampl.maxLod = VK_LOD_CLAMP_NONE;
		sampl.minLod = 0;

		sampl.magFilter = ExtractFilter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
		sampl.minFilter = ExtractFilter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

		sampl.mipmapMode = ExtractMipMapMode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));
		// Layouts seem to be deleted materialInstance->GetMaterialConstants()e

		VkSampler newSampler = nullptr;
		vkCreateSampler(rendererImpl->GetVulkanDevice(), &sampl, nullptr, &newSampler);

		samplers.push_back(newSampler);
	}

	meshAsset->CalculateTangentBasis();
	meshAsset->SetMeshBuffers(rendererImpl->UploadMesh(indexRef, vertexRef)); // Here the pipeline layout dies(?
	// Add the component to the entity
	auto* result = &entityRef.EmplaceComponent<MeshReference>(meshAsset);
	return result;
}

std::shared_ptr<Hush::GLTFMetallicRoughness> Hush::VulkanLoader::GenerateMaterial(
	size_t materialIdx, const fastgltf::Asset &asset, IRenderer *engine,
	DescriptorAllocatorGrowable &allocatorPool, const std::vector<GpuAllocatedImage> &loadedTextures)
{
	auto* rendererImpl = dynamic_cast<VulkanRenderer*>(engine);
	const fastgltf::Material &material = asset.materials.at(materialIdx);
	EMaterialPass passType = GltfLoadFunctions::GetMaterialPassFromFastGltfPass(material.alphaMode);

	auto materialInstance = std::make_shared<GLTFMetallicRoughness>();
	materialInstance->Init(rendererImpl);
	materialInstance->SetAlbedo(*reinterpret_cast<const glm::vec4 *>(&material.pbrData.baseColorFactor));
	materialInstance->SetEmissionColor(
		glm::vec3(material.emissiveFactor.x(), material.emissiveFactor.y(), material.emissiveFactor.z()));
	materialInstance->SetMetallicFactor(material.pbrData.metallicFactor);
	materialInstance->SetRoughnessFactor(material.pbrData.roughnessFactor);
	materialInstance->SetEmissionFactor(material.emissiveStrength);
	materialInstance->SetMaterialPass(passType);
	materialInstance->SetName(material.name);
	// Handle custom alpha cutoffs
	float alphaThreshold{};
	switch (passType)
	{
	case EMaterialPass::Other:
	case EMaterialPass::MainColor:
	case EMaterialPass::Transparent:
		alphaThreshold = 0.0F;
		break;
	case EMaterialPass::Mask:
		alphaThreshold = material.alphaCutoff;
		break;
	}

	materialInstance->SetAlphaThreshold(alphaThreshold);

	GLTFMetallicRoughness::MaterialResources &materialResources = materialInstance->GetMaterialResources();
	// default the material textures
	materialResources.colorImage = rendererImpl->GetDefaultImageProvider()->GetWhiteImage();
	materialResources.colorSampler = rendererImpl->GetDefaultSamplerLinear();
	materialResources.metalRoughImage = rendererImpl->GetDefaultImageProvider()->GetWhiteImage();
	materialResources.emissiveImage = rendererImpl->GetDefaultImageProvider()->GetTransparentImage();
	materialResources.normalImage = rendererImpl->GetDefaultImageProvider()->GetNormalImage();

	materialResources.metalRoughSampler = rendererImpl->GetDefaultSamplerLinear();
	materialResources.emissiveSampler = rendererImpl->GetDefaultSamplerLinear();
	materialResources.normalSampler = rendererImpl->GetDefaultSamplerLinear();

	// Then actually set them to the material's
	GltfLoadFunctions::SetMaterialTextures(materialInstance.get(), asset, material, &loadedTextures);

	// set the uniform buffer for the material data
	materialResources.dataBufferOffset = 0;
	materialInstance->GenerateMaterialInstance(&allocatorPool);
	return materialInstance;
}

std::optional<Hush::GpuAllocatedImage> Hush::VulkanLoader::LoadedTextureFromMaterial(
	const fastgltf::Asset &asset, const fastgltf::Material &material,
	const std::vector<GpuAllocatedImage> &loadedTextures)
{
	if (!material.pbrData.baseColorTexture.has_value())
	{
		return std::nullopt;
	}
	size_t textureDataIdx = material.pbrData.baseColorTexture->textureIndex;
	const fastgltf::Texture &fastgltfTexture = asset.textures.at(textureDataIdx);
	if (!fastgltfTexture.imageIndex.has_value())
	{
		return std::nullopt;
	}
	return loadedTextures.at(fastgltfTexture.imageIndex.value());
}

constexpr VkFilter Hush::VulkanLoader::ExtractFilter(const fastgltf::Filter &filter)
{
	switch (filter)
	{
	case fastgltf::Filter::Nearest:
	case fastgltf::Filter::NearestMipMapNearest:
	case fastgltf::Filter::NearestMipMapLinear:
		return VK_FILTER_NEAREST;

	case fastgltf::Filter::Linear:
	case fastgltf::Filter::LinearMipMapNearest:
	case fastgltf::Filter::LinearMipMapLinear:
	default:
		return VK_FILTER_LINEAR;
	}
}

constexpr VkSamplerMipmapMode Hush::VulkanLoader::ExtractMipMapMode(const fastgltf::Filter &filter)
{
	switch (filter)
	{
	case fastgltf::Filter::NearestMipMapNearest:
	case fastgltf::Filter::LinearMipMapNearest:
		return VK_SAMPLER_MIPMAP_MODE_NEAREST;

	case fastgltf::Filter::NearestMipMapLinear:
	case fastgltf::Filter::LinearMipMapLinear:
	default:
		return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
}
