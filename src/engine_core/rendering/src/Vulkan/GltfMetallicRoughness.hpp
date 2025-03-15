/*! \file GltfMetallicRoughness.hpp
	\author Kyn21kx
	\date 2024-05-31
	\brief Describes a GLTF asset to be loaded
*/

#pragma once
#include "Shared/IMaterial3D.hpp"
#include "VkTypes.hpp"
#include "VkDescriptors.hpp"
#include "VkMaterialInstance.hpp"
#include "Shared/MaterialPass.hpp"
#include <vulkan/vulkan_core.h>

namespace Hush
{
	class IRenderer;
	class GLTFMetallicRoughness : public IMaterial3D
	{
		VkMaterialPipeline m_opaquePipeline{};
		VkMaterialPipeline m_transparentPipeline{};

		VkDescriptorSetLayout m_materialLayout{};

	public:
		struct MaterialConstants
		{
			alignas(16) glm::vec4 colorFactors;
			alignas(16) glm::vec4 metalRoughFactors;
			alignas(4) float alphaThreshold;
			// padding, we need it anyway for uniform buffers
			char padding[12];
		};

		HUSH_STATIC_ASSERT(sizeof(MaterialConstants) == 48, "Metallic Roughness size mismatch!");

		struct MaterialResources
		{
			AllocatedImage colorImage;
			VkSampler colorSampler;
			AllocatedImage metalRoughImage;
			VkSampler metalRoughSampler;
			AllocatedImage normalImage;
			VkSampler normalSampler;
			VkBuffer dataBuffer;
			uint32_t dataBufferOffset;
		};

		DescriptorWriter writer;

		void BuildPipelines(IRenderer *engine, const std::string_view &fragmentShaderPath,
							const std::string_view &vertexShaderPath);

		void ClearResources(VkDevice device);

		[[nodiscard]]
		EAlphaBlendMode GetAlphaBlendMode() const noexcept override;

		void SetAlphaBlendMode(EAlphaBlendMode blendMode) noexcept override;

		[[nodiscard]]
		ECullMode GetCullMode() const noexcept override;

		void SetCullMode(ECullMode cullMode) override;

		inline VkMaterialInstance WriteMaterial(VkDevice device, EMaterialPass pass, const MaterialResources &resources,
												DescriptorAllocatorGrowable &descriptorAllocator)
		{

			Hush::VkMaterialInstance matData{};
			matData.passType = pass;

			switch (pass)
			{
			case Hush::EMaterialPass::Mask:

			case Hush::EMaterialPass::MainColor:
				matData.pipeline = &this->m_opaquePipeline;
				break;
			case Hush::EMaterialPass::Transparent:
				matData.pipeline = &this->m_transparentPipeline;
				break;
			default:
				HUSH_ASSERT(false, "Unkown material pass: {}", magic_enum::enum_name(pass));
				break;
			}

			// Not initialized material layout here from VkLoader
			matData.materialSet = descriptorAllocator.Allocate(device, this->m_materialLayout);

			writer.Clear();
			writer.WriteBuffer(0, resources.dataBuffer, sizeof(MaterialConstants), resources.dataBufferOffset,
							   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
			writer.WriteImage(1, resources.colorImage.imageView, resources.colorSampler,
							  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
			writer.WriteImage(2, resources.metalRoughImage.imageView, resources.metalRoughSampler,
							  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

			writer.WriteImage(3, resources.normalImage.imageView, resources.normalSampler,
							  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

			writer.UpdateSet(device, matData.materialSet);

			return matData;
		}
	};

} // namespace Hush
