#pragma once
#include "Entity.hpp"
#include "Ref.hpp"
#include "Shared/Mesh.hpp"
#include <filesystem>

namespace fastgltf
{
	class Asset;
	class Mesh;
} // namespace fastgltf

namespace Hush
{
	class ResourceManager;
}

namespace Hush::GLTFLoader
{

	void ProcessPrimitives(const fastgltf::Asset &asset, const fastgltf::Mesh &mesh, Hush::Ref<Hush::Mesh> &meshRef);

	Entity GenerateMeshEntities(Scene *activeScene, ResourceManager *resourceManager, const std::filesystem::path &path);

} // namespace Hush::GLTFLoader
