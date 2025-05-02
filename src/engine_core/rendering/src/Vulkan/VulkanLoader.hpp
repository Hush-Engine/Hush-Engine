#pragma once

#include "../../base/src/Common.hpp"
#include <vector>
#include <optional>
#include <memory>
#include <filesystem>
#include <fastgltf/types.hpp>
#include <Result.hpp>
#include "Shared/GpuAllocatedImage.hpp"
#include "Shared/ImageTexture.hpp"
#include "Vulkan/GltfMetallicRoughness.hpp"
#include "Shared/Mesh.hpp"
#include "../../core/src/Entity.hpp"

namespace Hush
{

	// forward declaration
	class VulkanRenderer;
	class GpuAllocatedBuffer;

	// TODO: Make non-static
	class VulkanLoader
	{

	public:
		enum class EError
		{
			None = 0,
			FileNotFound,
			InvalidMeshFile,
			FormatNotSupported
		};

		static Result<std::vector<Entity>, EError> LoadGltfMeshes(
			VulkanRenderer *engine, std::filesystem::path filePath, Scene *activeScene);

		static GpuAllocatedImage LoadTexture(VulkanRenderer *engine, const ImageTexture &texture);

	private:
		static std::vector<GpuAllocatedImage> LoadAllTextures(const fastgltf::Asset &asset, VulkanRenderer *engine);

		static Mesh* CreateMeshFromGltfMesh(const fastgltf::Mesh &mesh, const fastgltf::Asset &asset,
													 Entity &entityRef, VulkanRenderer *engine);

		static std::shared_ptr<GLTFMetallicRoughness> GenerateMaterial(
			size_t materialIdx, const fastgltf::Asset &asset, VulkanRenderer *engine,
			GpuAllocatedBuffer *sceneMaterialBuffer, DescriptorAllocatorGrowable &allocatorPool,
			const std::vector<GpuAllocatedImage> &loadedTextures);

		static std::optional<GpuAllocatedImage> LoadedTextureFromMaterial(
			const fastgltf::Asset &asset, const fastgltf::Material &material,
			const std::vector<GpuAllocatedImage> &loadedTextures);

		static constexpr VkFilter ExtractFilter(const fastgltf::Filter &filter);
		static constexpr VkSamplerMipmapMode ExtractMipMapMode(const fastgltf::Filter &filter);
	};

} // namespace Hush
