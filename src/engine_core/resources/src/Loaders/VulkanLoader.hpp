#pragma once

#include <vector>
#include <optional>
#include <memory>
#include <filesystem>
#include <fastgltf/types.hpp>
#include <Result.hpp>
#include "Loaders/IModelLoader.hpp"
#include "Renderer.hpp"
#include "Shared/GpuAllocatedImage.hpp"
#include "Shared/ImageTexture.hpp"
#include "Vulkan/GltfMetallicRoughness.hpp"
#include "../../core/src/Entity.hpp"

namespace Hush
{

	// forward declaration
	class GpuAllocatedBuffer;
	class ResourceManager;
	class MeshReference;

	class VulkanLoader final : public IModelLoader
	{
	public:
		VulkanLoader() = default;

		void SetResourceManager(ResourceManager *resourceManager) override;

		Result<std::vector<Entity>, EError> LoadMeshes(IRenderer *engine, const std::filesystem::path &filePath,
													   Scene *activeScene) override;

		GpuAllocatedImage LoadTexture(IRenderer *engine, const ImageTexture &texture) override;

		[[nodiscard]]
		ResourceManager *GetResourceManager() const override;

	private:
		std::vector<GpuAllocatedImage> LoadAllTextures(const fastgltf::Asset &asset, IRenderer *engine);

		MeshReference *CreateMeshFromGltfMesh(const fastgltf::Mesh &mesh, const fastgltf::Asset &asset,
											  Entity &entityRef, IRenderer *engine);

		std::shared_ptr<GLTFMetallicRoughness> GenerateMaterial(size_t materialIdx, const fastgltf::Asset &asset,
																IRenderer *engine,
																DescriptorAllocatorGrowable &allocatorPool,
																const std::vector<GpuAllocatedImage> &loadedTextures);

		std::optional<GpuAllocatedImage> LoadedTextureFromMaterial(
			const fastgltf::Asset &asset, const fastgltf::Material &material,
			const std::vector<GpuAllocatedImage> &loadedTextures);

		constexpr VkFilter ExtractFilter(const fastgltf::Filter &filter);
		constexpr VkSamplerMipmapMode ExtractMipMapMode(const fastgltf::Filter &filter);

		ResourceManager *m_resourceManager = nullptr;
	};

} // namespace Hush
