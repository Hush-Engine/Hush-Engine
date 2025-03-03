#define VK_NO_PROTOTYPES
#include <volk.h>
#include "VulkanLoader.hpp"
#include <fastgltf/types.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/core.hpp>
#include "VkTypes.hpp"
#include "VulkanRenderer.hpp"
#include "Shared/ImageTexture.hpp"
#include "VkMaterialInstance.hpp"
#include "VulkanMeshNode.hpp"
#include "VulkanAllocatedBuffer.hpp"
#include "vk_mem_alloc.hpp"
#include "Shared/GltfLoadFunctions.hpp"

Hush::Result<std::vector<std::shared_ptr<Hush::VulkanMeshNode>>, Hush::VulkanLoader::EError> Hush::VulkanLoader::LoadGltfMeshes(VulkanRenderer* engine, std::filesystem::path filePath)
{
	if (!std::filesystem::exists(filePath))
    {
        return EError::FileNotFound;
    }

    fastgltf::Expected<fastgltf::GltfDataBuffer> loadedData = fastgltf::GltfDataBuffer::FromPath(filePath);

    if (!loadedData)
    {
        return EError::InvalidMeshFile;
    }

    fastgltf::GltfDataBuffer &data = loadedData.get();

    constexpr fastgltf::Options loadingOptions = fastgltf::Options::LoadExternalBuffers;

    fastgltf::Parser parser{};

    fastgltf::Expected<fastgltf::Asset> loadedAsset = parser.loadGltfBinary(data, filePath.parent_path(), loadingOptions);

	HUSH_ASSERT(loadedAsset, "GLTF asset at {} not properly loaded, error: {}!", filePath.string(), fastgltf::getErrorMessage(loadedAsset.error()));
	
	std::vector<uint32_t> indices;
	std::vector<Vertex> vertices;
	std::vector<std::shared_ptr<VulkanMeshNode>> meshes;
	std::vector<std::shared_ptr<VulkanMeshNode>> rootNodes;
	//TODO: render these meshes instead of the loaded nodes, or store these in there idk
	for (const fastgltf::Mesh& mesh : loadedAsset->meshes)
	{
		auto node = std::make_shared<VulkanMeshNode>(CreateMeshFromGltfMesh(mesh, loadedAsset.get(), indices, vertices, engine));
		node->SetLocalTransform(glm::mat4{ 1.F });
		node->SetWorldTransform(glm::mat4{ 1.F });
		meshes.emplace_back(node);
	}


	for (const fastgltf::Node& node : loadedAsset->nodes)
	{
		if (!node.meshIndex.has_value()) continue;
		std::shared_ptr<VulkanMeshNode> meshNode = meshes.at(node.meshIndex.value());
		meshNode->SetLocalTransform(GltfLoadFunctions::GetNodeTransform(node));
	}

	for (int i = 0; i < loadedAsset->nodes.size(); i++) {
		fastgltf::Node& node = loadedAsset->nodes[i];
		if (!node.meshIndex.has_value()) continue;
		std::shared_ptr<VulkanMeshNode>& sceneNode = meshes[node.meshIndex.value()];
		if (node.children.empty()) {
			//If there are no children, we'll set the transform to global(???
			sceneNode->SetWorldTransform(sceneNode->GetLocalTransform());
			continue;
		}
		for (size_t& c : node.children) {
			sceneNode->AddChild(meshes[c]);
			meshes[c]->SetParent(sceneNode);
		}
	}

	return meshes;
}

AllocatedImage Hush::VulkanLoader::LoadTexture(VulkanRenderer* engine, const ImageTexture& texture)
{	
	VkExtent3D extent{};
	extent.width = texture.GetWidth();
	extent.height = texture.GetHeight();
	extent.depth = 1;

	constexpr VkFormat defaultImageFormat = VK_FORMAT_R8G8B8A8_UNORM;
	
	return engine->CreateImage(texture.GetImageData(), extent, defaultImageFormat, VK_IMAGE_USAGE_SAMPLED_BIT);
}

std::vector<AllocatedImage> Hush::VulkanLoader::LoadAllTextures(const fastgltf::Asset& asset, VulkanRenderer* engine)
{
	std::vector<AllocatedImage> loadedTexturesResult;
	loadedTexturesResult.reserve(asset.images.size());
	for (const fastgltf::Image& image : asset.images) {
		std::shared_ptr<ImageTexture> texture = GltfLoadFunctions::TextureFromImageDataSource(asset, image);
		AllocatedImage loadedImage = LoadTexture(engine, *texture.get());
		loadedTexturesResult.emplace_back(loadedImage);
	}
	return loadedTexturesResult;
}

Hush::VulkanMeshNode Hush::VulkanLoader::CreateMeshFromGltfMesh(const fastgltf::Mesh& mesh, const fastgltf::Asset& asset, std::vector<uint32_t>& indicesRef, std::vector<Vertex>& verticesRef, VulkanRenderer* engine)
{
	VulkanMeshNode meshNode(std::make_shared<MeshAsset>());

	MeshAsset& meshAsset = meshNode.GetMesh();

	meshAsset.name = mesh.name;
	//Clear out the vector buffers
	indicesRef.clear();
	verticesRef.clear();

	VulkanAllocatedBuffer materialDataBuffer(
		static_cast<uint32_t>(sizeof(GLTFMetallicRoughness::MaterialConstants) * asset.materials.size()),
		VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT,
		VMA_MEMORY_USAGE_CPU_TO_GPU,
		engine->GetVmaAllocator()
	);

	//TODO: constexpr(?
	const std::vector<DescriptorAllocatorGrowable::PoolSizeRatio> sizes = {
		{ VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER, 3 },
		{ VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER, 3 },
		{ VK_DESCRIPTOR_TYPE_STORAGE_BUFFER, 1 }
	};

	
	meshNode.m_descriptorPool.Init(volkGetLoadedDevice(), static_cast<uint32_t>(asset.materials.size()), sizes);

	std::vector<AllocatedImage> loadedTextures = LoadAllTextures(asset, engine);
	std::vector<std::shared_ptr<VkMaterialInstance>> loadedMaterials;

	for (const fastgltf::Primitive& primitive : mesh.primitives)
	{
		size_t initialVertex = verticesRef.size();
		GeoSurface surfaceToAdd{};
		surfaceToAdd.startIndex = static_cast<uint32_t>(indicesRef.size());
		const fastgltf::Accessor& primitiveIdxAccessor = asset.accessors[primitive.indicesAccessor.value()];
		surfaceToAdd.count = static_cast<uint32_t>(primitiveIdxAccessor.count);

		//TODO: Make a function that can load any arbitrary primitive from glTF

		// load indexes
		indicesRef.reserve(indicesRef.size() + primitiveIdxAccessor.count);
		fastgltf::iterateAccessor<uint32_t>(asset, primitiveIdxAccessor, [&](uint32_t idx) {
			indicesRef.push_back(idx + static_cast<uint32_t>(initialVertex));
			});

		std::vector<glm::vec3> vertexBuffer = GltfLoadFunctions::FindAttributeByName<glm::vec3>(primitive, asset, "POSITION");
		for (const glm::vec3& v : vertexBuffer) {
			Vertex vertexToAdd{};
			vertexToAdd.position = v;
			verticesRef.push_back(vertexToAdd);
		}

		std::vector<glm::vec3> normalBuffer = GltfLoadFunctions::FindAttributeByName<glm::vec3>(primitive, asset, "NORMAL");
		for (uint32_t i = 0; i < normalBuffer.size(); i++) {
			verticesRef.at(i + initialVertex).normal = normalBuffer.at(i);
		}

		// load UVs
		std::vector<glm::vec2> texBuffer = GltfLoadFunctions::FindAttributeByName<glm::vec2>(primitive, asset, "TEXCOORD_0");

		for (uint32_t i = 0; i < texBuffer.size(); i++) {
			verticesRef.at(i + initialVertex).uv_x = texBuffer.at(i).x;
			verticesRef.at(i + initialVertex).uv_y = texBuffer.at(i).y;
		}

		// load vertex colors
		std::vector<glm::vec4> colors = GltfLoadFunctions::FindAttributeByName<glm::vec4>(primitive, asset, "COLOR_0");

		for (uint32_t i = 0; i < colors.size(); i++) {
			verticesRef.at(i + initialVertex).color = colors.at(i);
		}

		if (primitive.materialIndex.has_value()) {
			size_t materialIdx = primitive.materialIndex.value();
			std::shared_ptr<VkMaterialInstance> materialInstance = GenerateMaterial(materialIdx, asset, engine, &materialDataBuffer, meshNode.m_descriptorPool, loadedTextures);
			surfaceToAdd.material = materialInstance;
			loadedMaterials.emplace_back(materialInstance);
		}

		meshAsset.surfaces.emplace_back(std::move(surfaceToAdd));
	}

	std::vector<VkSampler> samplers;

	for (const fastgltf::Sampler& sampler : asset.samplers) {
		//Layouts seem to be deleted here
		VkSamplerCreateInfo sampl = { .sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO, .pNext = nullptr };
		sampl.maxLod = VK_LOD_CLAMP_NONE;
		sampl.minLod = 0;

		sampl.magFilter = ExtractFilter(sampler.magFilter.value_or(fastgltf::Filter::Nearest));
		sampl.minFilter = ExtractFilter(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

		sampl.mipmapMode = ExtractMipMapMode(sampler.minFilter.value_or(fastgltf::Filter::Nearest));

		VkSampler newSampler;
		vkCreateSampler(engine->GetVulkanDevice(), &sampl, nullptr, &newSampler);

		samplers.push_back(newSampler);
	}


	//Load mats first, then apply those to the primitives
	//for (size_t materialIdx = 0; materialIdx < asset.materials.size(); materialIdx++) {
	//}

	meshAsset.meshBuffers = engine->UploadMesh(indicesRef, verticesRef); //Here the pipeline layout dies(?
	meshNode.SetMaterialDataBuffer(materialDataBuffer);
	return meshNode;
}

std::shared_ptr<Hush::VkMaterialInstance> Hush::VulkanLoader::GenerateMaterial(size_t materialIdx, const fastgltf::Asset& asset, VulkanRenderer* engine, VulkanAllocatedBuffer* sceneMaterialBuffer, DescriptorAllocatorGrowable& allocatorPool, const std::vector<AllocatedImage>& loadedTextures)
{
	const fastgltf::Material& material = asset.materials.at(materialIdx);

	GLTFMetallicRoughness::MaterialConstants constants;
	HUSH_STATIC_ASSERT(
		sizeof(constants.colorFactors) == sizeof(material.pbrData.baseColorFactor),
		"Material constants' colors are not the same size as fastgltf pbr data colors, make sure fastgltf is compiled with using num = float"
	);
	constants.colorFactors = *reinterpret_cast<const glm::vec4*>(&material.pbrData.baseColorFactor);

	constants.metalRoughFactors.x = material.pbrData.metallicFactor;
	constants.metalRoughFactors.y = material.pbrData.roughnessFactor;

	//Scene Material buffer writing
	VmaAllocationInfo& allocInfo = sceneMaterialBuffer->GetAllocationInfo();
	GLTFMetallicRoughness::MaterialConstants* mappedData = static_cast<GLTFMetallicRoughness::MaterialConstants*>(allocInfo.pMappedData);

	EMaterialPass passType = GltfLoadFunctions::GetMaterialPassFromFastGltfPass(material.alphaMode);
	
	//Handle custom alpha cutoffs
	switch (passType)
	{
	case EMaterialPass::MainColor:
	case EMaterialPass::Transparent:
		constants.alphaThreshold = 0.0f;
		break;
	case EMaterialPass::Mask:
		constants.alphaThreshold = material.alphaCutoff;
	}

	mappedData[materialIdx] = constants;
	GLTFMetallicRoughness::MaterialResources materialResources;
	// default the material textures
	std::optional<AllocatedImage> loadedTextureToUse = LoadedTextureFromMaterial(asset, material, loadedTextures);
	materialResources.colorImage = loadedTextureToUse.has_value() ? loadedTextureToUse.value() : engine->GetDefaultWhiteImage();
	materialResources.colorSampler = engine->GetDefaultSamplerLinear();
	materialResources.metalRoughImage = engine->GetDefaultWhiteImage();
	materialResources.metalRoughSampler = engine->GetDefaultSamplerLinear();

	// set the uniform buffer for the material data
	materialResources.dataBuffer = sceneMaterialBuffer->GetBuffer();
	materialResources.dataBufferOffset = static_cast<uint32_t>(materialIdx * sizeof(GLTFMetallicRoughness::MaterialConstants));
	GLTFMetallicRoughness& metallicMatInstance = engine->GetMetalRoughMaterial();
	return std::make_shared<VkMaterialInstance>(metallicMatInstance.WriteMaterial(engine->GetVulkanDevice(), passType, materialResources, allocatorPool));
}


std::optional<AllocatedImage> Hush::VulkanLoader::LoadedTextureFromMaterial(const fastgltf::Asset& asset, const fastgltf::Material& material, const std::vector<AllocatedImage>& loadedTextures)
{
	if (!material.pbrData.baseColorTexture.has_value()) {
		return std::nullopt;
	}
	size_t textureDataIdx = material.pbrData.baseColorTexture->textureIndex;
	const fastgltf::Texture& fastgltfTexture = asset.textures.at(textureDataIdx);
	if (!fastgltfTexture.imageIndex.has_value()) {
		return std::nullopt;
	}
	return loadedTextures.at(fastgltfTexture.imageIndex.value());
}

constexpr VkFilter Hush::VulkanLoader::ExtractFilter(const fastgltf::Filter& filter)
{
	switch (filter) {
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

constexpr VkSamplerMipmapMode Hush::VulkanLoader::ExtractMipMapMode(const fastgltf::Filter& filter)
{
	switch (filter) {
		case fastgltf::Filter::NearestMipMapNearest:
		case fastgltf::Filter::LinearMipMapNearest:
			return VK_SAMPLER_MIPMAP_MODE_NEAREST;

		case fastgltf::Filter::NearestMipMapLinear:
		case fastgltf::Filter::LinearMipMapLinear:
		default:
			return VK_SAMPLER_MIPMAP_MODE_LINEAR;
	}
}
