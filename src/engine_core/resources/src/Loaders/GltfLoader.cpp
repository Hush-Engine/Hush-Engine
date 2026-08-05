#include "GltfLoader.hpp"
#include "Assertions.hpp"
#include "Components/LocalTransform.hpp"
#include "Components/Material3D.hpp"
#include "Components/MeshReference.hpp"
#include "Components/WorldTransform.hpp"
#include "Components/GpuUploadComponent.hpp"
#include <fastgltf/tools.hpp>
#include <magic_enum/magic_enum.hpp>
#include <vector>
#include <fstream>
#include <optional>
#include "RHI/IGraphicsDevice.hpp"
#include "Ref.hpp"
#include "ResourceManager.hpp"
#include <fastgltf/types.hpp>
#include "GltfLoadFunctions.hpp"
#include "Scene.hpp"

void Hush::GLTFLoader::ProcessPrimitives(const RenderingContext &renderingContext, const fastgltf::Asset &asset,
										 const fastgltf::Mesh &mesh, MeshReference &meshRef,
										 const std::filesystem::path &basePath)
{
	Ref<Mesh> &innerMeshRef = meshRef.GetMesh();
	std::vector<uint32_t> &indexRef = innerMeshRef->GetIndexBuffer();
	std::vector<Mesh::Vertex> &vertexRef = innerMeshRef->GetVertexBuffer();
	indexRef.clear();
	vertexRef.clear();

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
				MakeMaterial(renderingContext, materialIdx, asset, {}, meshRef, basePath);
			// Keep alive on the Mesh component
			meshRef.PushMaterial(materialInstance);
			// Non-owning ref on the surface
			surfaceToAdd.material = materialInstance.Get();
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
													const std::filesystem::path &path)
{
	// Open the file and parse it with the gltf loader functions
	auto assetRes = GltfLoadFunctions::GetAssetFromFile(path);
	HUSH_COND_FAIL_MSG_V(assetRes, Entity::Null(), "Could not load mesh at {}, error: {}", path.string(),
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
		Ref<Mesh> meshRef =
			resourceManager->AllocateRef<Mesh>(path.string());
		auto &meshComponent = entity.EmplaceComponent<MeshReference>(meshRef);

		meshRef->SetName(mesh.name);

		std::filesystem::path basePath = path.parent_path();
		ProcessPrimitives(renderingContext, assetRes.get(), mesh, meshComponent, basePath);

		// Generate the material per primitive here

		meshRef->CalculateTangentBasis();
		// meshRef->SetMeshBuffers(rendererImpl->UploadMesh(indexRef, vertexRef)); // Here the pipeline layout dies(?

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
		fatherEntity = activeScene->CreateEntityWithName(path.stem().string());
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
	const std::vector<Graphics::IGraphicsTexture *> &loadedTextures, MeshReference &meshRef,
	const std::filesystem::path &basePath)
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
															   material.emissiveFactor.z(), 1.0f));
	materialInstance->SetProperty("alphaCutoff", material.alphaCutoff);
	constexpr uint32_t useNormalsFlag = 1;
	materialInstance->SetProperty("optionFlags", useNormalsFlag);
	materialInstance->SetName(material.name);

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
		auto &texRefs = meshRef.GetMaterialTextureRefs()[materialInstance.Get()];
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
