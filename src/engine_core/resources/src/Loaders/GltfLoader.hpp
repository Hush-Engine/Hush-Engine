#pragma once
#include "Components/Material3D.hpp"
#include "Entity.hpp"
#include "Ref.hpp"
#include "Shared/Mesh.hpp"
#include <filesystem>
#include <vector>

namespace fastgltf
{
	class Asset;
	class Mesh;
} // namespace fastgltf

namespace Hush
{
	class ResourceManager;
	class MeshReference;
	namespace Graphics
	{
		class IGraphicsTexture;
		class IGraphicsDevice;
		class Material3D;
		struct Material3DDescriptor;
	} // namespace Graphics

	// TODO: Move to its own file
	struct RenderingContext
	{
		const Graphics::Material3DDescriptor *materialDescriptor;
		Scene *activeScene;
		ResourceManager *resourceManager;
		Graphics::IGraphicsDevice *device;
	};

} // namespace Hush

namespace Hush::GLTFLoader
{

	void ProcessPrimitives(const RenderingContext &renderingContext, const fastgltf::Asset &asset,
						   const fastgltf::Mesh &mesh, Hush::MeshReference &meshRef);

	Entity GenerateMeshEntities(const RenderingContext &renderingContext, const std::filesystem::path &path);

	// Maybe make the textures vector a vector of Ref<Texture>
	Ref<Graphics::Material3D> MakeMaterial(const RenderingContext &renderingContext, size_t materialIdx,
										   const fastgltf::Asset &asset,
										   const std::vector<Graphics::IGraphicsTexture *> &loadedTextures);

} // namespace Hush::GLTFLoader
