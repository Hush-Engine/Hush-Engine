#pragma once
#include "Components/Material3D.hpp"
#include "Entity.hpp"
#include "Loaders/GltfLoadFunctions.hpp"
#include "Ref.hpp"
#include "Shared/Mesh.hpp"
#include "VirtualFilesystem.hpp"
#include <cstdint>
#include <filesystem>
#include <optional>
#include <span>
#include <string_view>
#include <type_traits>
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
		Scene *activeScene; // Optional for asset cooking pipeline, required for direct loads where we instance comps
		ResourceManager *resourceManager;
		VirtualFilesystem *virtualFilesystem;
		Graphics::IGraphicsDevice *device;
	};

} // namespace Hush

namespace Hush::GLTFLoader
{

	// Opaque handle into the fastgltf::Asset struct
	struct AssetHandle
	{
		constexpr static size_t ASSET_CONTAINER_SIZE = 584;
		constexpr static size_t ASSET_CONTAINER_ALIGN = 8;
		alignas(ASSET_CONTAINER_ALIGN) std::array<std::byte, ASSET_CONTAINER_SIZE> backing;

		[[nodiscard]]
		size_t MeshCount() const;

		[[nodiscard]]
		size_t PrimitiveCount(size_t meshIndex) const;
	};

	// HACK: Refactor the Material and Texture info to a stream of entries with binding names, the current setup only
	// works if we assume the mesh is running the default PBR shader

	/// @brief Describes all relevant information about the default PBR GLTF material, this is NOT meant for rendering
	struct MaterialInfo
	{
		// 63 + null
		static constexpr size_t MAX_MAT_NAME = 64;
		uint32_t resource;
		glm::vec4 albedo;
		char name[MAX_MAT_NAME];
	};

	/// @brief Indicates either an asset id (cooked texture) or an offset into the original glb file where the texture
	/// data resides
	struct TextureInfo
	{
		static constexpr size_t MAX_TEX_NAME = 64;
		/// @brief If not 0, this is the cooked texture this texture points to
		uint32_t resource;
		/// @brief Byte offset of the texture data within the original glb file
		uint64_t offset;
		/// @brief Byte length of the texture data within the original glb file
		uint64_t size;
		int32_t width;
		int32_t height;
		char name[MAX_TEX_NAME];
	};

	void FillMeshData(AssetHandle *asset, size_t meshIndex, std::vector<Mesh::Vertex> *outVertexBuffer,
					  std::vector<uint32_t> *outIndexBuffer, std::vector<MaterialInfo> *outMaterials,
					  std::vector<std::vector<TextureInfo>> *outTexturesByMat, std::vector<GeoSurface> *outSurfaces);

	GltfLoadFunctions::EError LoadAssetFromBinary(std::span<const std::byte> data, AssetHandle *outAsset);

	void ProcessPrimitives(const RenderingContext &renderingContext, const fastgltf::Asset &asset,
						   const fastgltf::Mesh &mesh, const std::filesystem::path &basePath, Ref<Mesh> &innerMeshRef,
						   MeshReference *meshRef = nullptr); // A MeshRefcomponent may or may not be present, for
															  // either the entity or raw upload paths

	Entity GenerateMeshEntities(const RenderingContext &renderingContext, const std::string_view &virtualPath);

	/// @brief Loads and allocates all necessary meshes on the resource manager, these will be kept alive just until the
	/// end of the frame if no entity claims them, so, be sure to do so
	bool LoadMeshes(const RenderingContext &renderingContext, const std::string_view &path);

	/// @brief Creates all Mesh references needed to instance the given binary model, it does not associate the data of
	/// the component with any entities, raw data out
	std::vector<MeshReference> LoadMeshesFromBinary(std::span<const std::byte> binary,
													const RenderingContext &renderingContext);

	// Maybe make the textures vector a vector of Ref<Texture>
	Ref<Graphics::Material3D> MakeMaterial(const RenderingContext &renderingContext, size_t materialIdx,
										   const fastgltf::Asset &asset,
										   const std::vector<Graphics::IGraphicsTexture *> &loadedTextures,
										   const std::filesystem::path &basePath, MeshReference *meshRefComp = nullptr);

} // namespace Hush::GLTFLoader
