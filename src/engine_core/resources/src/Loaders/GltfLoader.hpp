#pragma once
#include "Components/Material3D.hpp"
#include "Entity.hpp"
#include "Ref.hpp"
#include "Shared/Mesh.hpp"
#include <filesystem>
#include <string_view>
#include <vector>

namespace fastgltf
{
	class Asset;
	struct Mesh;
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
						   const fastgltf::Mesh &mesh, Hush::MeshReference &meshRef,
						   const std::filesystem::path &basePath);

	Entity GenerateMeshEntities(const RenderingContext &renderingContext, const std::string_view &path);

	/// @brief Loads and allocates all necessary meshes on the resource manager, these will be kept alive just until the end of the frame if no entity claims them, so, be sure to do so
	bool LoadMeshes(const RenderingContext& renderingContext, const std::string_view& path);

	// Maybe make the textures vector a vector of Ref<Texture>
	Ref<Graphics::Material3D> MakeMaterial(const RenderingContext &renderingContext, size_t materialIdx,
										   const fastgltf::Asset &asset,
										   const std::vector<Graphics::IGraphicsTexture *> &loadedTextures,
										   Hush::MeshReference &meshRef, const std::filesystem::path &basePath);

} // namespace Hush::GLTFLoader
