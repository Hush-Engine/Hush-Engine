#include "GltfLoader.hpp"
#include "Components/LocalTransform.hpp"
#include "Components/MeshReference.hpp"
#include "Components/WorldTransform.hpp"
#include "Components/GpuUploadComponent.hpp"
#include <fastgltf/tools.hpp>
#include <vector>
#include "ResourceManager.hpp"
#include <fastgltf/types.hpp>
#include "GltfLoadFunctions.hpp"
#include "Scene.hpp"

void Hush::GLTFLoader::ProcessPrimitives(const fastgltf::Asset &asset, const fastgltf::Mesh &mesh, Hush::Ref<Hush::Mesh> &meshRef)
{
	std::vector<uint32_t> &indexRef = meshRef->GetIndexBuffer();
	std::vector<Mesh::Vertex> &vertexRef = meshRef->GetVertexBuffer();
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

		// load vertex colors
		std::vector<glm::vec4> colors = GltfLoadFunctions::FindAttributeByName<glm::vec4>(primitive, asset, "COLOR_0");

		for (uint32_t i = 0; i < colors.size(); i++)
		{
			vertexRef.at(i + initialVertex).color = colors.at(i);
		}

		// Correct normals if empty
		if (normalBuffer.empty())
		{
			meshRef->CalculateNormals();
		}

		meshRef->AddSurface(std::move(surfaceToAdd));
	}
}

/// @brief This is a temporary function, we need to move this behavior to HushCooker, but this will work to prove we can
/// already load and render objects
Hush::Entity Hush::GLTFLoader::GenerateMeshEntities(Hush::Scene *activeScene, Hush::ResourceManager *resourceManager,
								  const std::filesystem::path &path)
{
	// Open the file and parse it with the gltf loader functions
	auto assetRes = GltfLoadFunctions::GetAssetFromFile(path);
	HUSH_COND_FAIL_MSG_V(assetRes, Entity::Null(), "Could not load mesh at {}, error: {}", path.string(),
						 magic_enum::enum_name(assetRes.error()));
	std::vector<Entity> entities;

	// For each node, generate an entity with a transform

	int32_t generatedEntities = 0;
	for (const fastgltf::Mesh &mesh : assetRes->meshes)
	{
		Entity entity = activeScene->CreateEntityWithName(mesh.name.empty() ? "LoadedMesh" : mesh.name);
		entity.AddComponent<WorldTransform>();
		entity.AddComponent<LocalTransform>();
		// Create the mesh
		Ref<Mesh> meshRef = resourceManager->AllocateRef<Mesh>(mesh.name);

		meshRef->SetName(mesh.name);

		ProcessPrimitives(assetRes.get(), mesh, meshRef);

		// Generate the material per primitive here

		meshRef->CalculateTangentBasis();
		// meshRef->SetMeshBuffers(rendererImpl->UploadMesh(indexRef, vertexRef)); // Here the pipeline layout dies(?

		// Load everything into Mesh components
		// Add a GpuUploadComponent for the UploadResourceSystem to pick it up
		entity.EmplaceComponent<MeshReference>(meshRef);
		entity.AddComponent<Renderer::GpuUploadComponent>();

		entities.emplace_back(std::move(entity));
		generatedEntities++;
	}

	Entity fatherEntity = Entity::Null();

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
