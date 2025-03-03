#pragma once

//TODO: Move these undefs to the common.hpp file after dev merge
// remove stupid MSVC min/max macro definitions
#ifdef WIN32
	#undef min
	#undef max
#endif

#include <vector>
#include <string>
#include <optional>
#include <memory>
#include <filesystem>
#include "GPUMeshBuffers.hpp"
#include <fastgltf/types.hpp>
#include <Result.hpp>
#include "VkMaterialInstance.hpp"
#include "Shared/ImageTexture.hpp"
#include "VulkanMeshNode.hpp"

namespace Hush {

	struct GeoSurface {
		uint32_t startIndex;
		uint32_t count;
		std::shared_ptr<VkMaterialInstance> material;
	};

	struct MeshAsset {
		std::string name;

		std::vector<GeoSurface> surfaces;
		GPUMeshBuffers meshBuffers;
	};

	//forward declaration
	class VulkanRenderer;
	class VulkanAllocatedBuffer;

	// TODO: Make non-static
	class VulkanLoader {

	public:
		enum class EError {
			None = 0,
			FileNotFound,
			InvalidMeshFile,
			FormatNotSupported
		};

		static Result<std::vector<std::shared_ptr<VulkanMeshNode>>, EError> LoadGltfMeshes(VulkanRenderer* engine, std::filesystem::path filePath);

		static AllocatedImage LoadTexture(VulkanRenderer* engine, const ImageTexture& texture);

	private:

		static std::vector<AllocatedImage> LoadAllTextures(const fastgltf::Asset& asset, VulkanRenderer* engine);

		static VulkanMeshNode CreateMeshFromGltfMesh(const fastgltf::Mesh& mesh, const fastgltf::Asset& accessors, std::vector<uint32_t>& indicesRef, std::vector<Vertex>& verticesRef, VulkanRenderer* engine);

		static std::shared_ptr<VkMaterialInstance> GenerateMaterial(size_t materialIdx, const fastgltf::Asset& asset, VulkanRenderer* engine, VulkanAllocatedBuffer* sceneMaterialBuffer, DescriptorAllocatorGrowable& allocatorPool, const std::vector<AllocatedImage>& loadedTextures);

		static std::optional<AllocatedImage> LoadedTextureFromMaterial(const fastgltf::Asset& asset, const fastgltf::Material& material, const std::vector<AllocatedImage>& loadedTextures);

		static constexpr VkFilter ExtractFilter(const fastgltf::Filter& filter);
		static constexpr VkSamplerMipmapMode ExtractMipMapMode(const fastgltf::Filter& filter);
	};

}