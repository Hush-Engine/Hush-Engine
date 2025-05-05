/*! \file GltfMetallicRoughness.hpp
	\author Kyn21kx
	\date 2024-05-31
	\brief Describes a GLTF asset to be loaded
*/

#pragma once
#include "Shared/GpuAllocatedBuffer.hpp"
#include "Shared/GpuAllocatedImage.hpp"
#include "Shared/IMaterial3D.hpp"
#include "Shared/MaterialOptions.hpp"
#include "Shared/Types/MaterialInstance.hpp"
#include "VkDescriptors.hpp"
#include "VkMaterialInstance.hpp"
#include "Shared/MaterialPass.hpp"
#include <cstdint>
#include <glm/ext/vector_float4.hpp>
#include <memory>
#include <vulkan/vulkan_core.h>

namespace Hush
{
	class IRenderer;
	class GLTFMetallicRoughness final : public IMaterial3D
	{
	private:
		VkMaterialPipeline m_opaquePipeline{};
		VkMaterialPipeline m_transparentPipeline{};

		VkDescriptorSetLayout m_materialLayout{};

	public:
		struct MaterialConstants
		{
			alignas(16) glm::vec4 colorFactors;
			alignas(16) glm::vec4 metalRoughFactors;
			alignas(16) glm::vec4 emissionFactors; // Vec3 for color, w for intensity
			alignas(4) float alphaThreshold;
			// padding, we need it anyway for uniform buffers
			char padding[12];
		};

		HUSH_STATIC_ASSERT(sizeof(MaterialConstants) % 16 == 0, "Metallic Roughness size mismatch!");

		struct MaterialResources
		{
			GpuAllocatedImage colorImage;
			VkSampler colorSampler;
			GpuAllocatedImage metalRoughImage;
			VkSampler metalRoughSampler;
			GpuAllocatedImage normalImage;
			VkSampler normalSampler;
			GpuAllocatedImage emissiveImage;
			VkSampler emissiveSampler;
			GpuAllocatedBuffer gpuDataBuffer;
			VkBuffer dataBuffer;
			uint32_t dataBufferOffset;
		};

		GLTFMetallicRoughness() = default;

		DescriptorWriter writer;

		// TODO: Maybe make a version that does not require a previous material buffer
		void Init(IRenderer *renderer, GpuAllocatedBuffer materialBuffer, size_t materialIdx, uint32_t dataBufferOffset = 0);

		void ClearResources(VkDevice device);

		[[nodiscard]]
		EAlphaBlendMode GetAlphaBlendMode() const noexcept override;

		void SetAlphaBlendMode(EAlphaBlendMode blendMode) noexcept override;

		[[nodiscard]]
		EMaterialPass GetMaterialPass() const noexcept override;

		void SetMaterialPass(EMaterialPass pass) override;

		[[nodiscard]]
		ECullMode GetCullMode() const noexcept override;

		void SetCullMode(ECullMode cullMode) override;

		void GenerateMaterialInstance(DescriptorAllocatorGrowable *descriptorAllocator);

		[[nodiscard]]
		const glm::vec4 &GetAlbedo() const noexcept;

		glm::vec4 &GetAlbedo() noexcept;

		void SetAlbedo(const glm::vec4 &color) noexcept;

		[[nodiscard]]
		const glm::vec3 &GetEmissionColor() const noexcept;
		
		glm::vec3 &GetEmissionColor() noexcept;

		void SetEmissionColor(const glm::vec3 &color) noexcept;

		[[nodiscard]]
		const float &EmissionFactor() const noexcept;

		void SetEmissionFactor(float emissionFactor) noexcept;

		[[nodiscard]]
		const float &GetMetallicFactor() const noexcept;

		void SetMetallicFactor(float factor) noexcept;

		[[nodiscard]]
		const float &GetRoughnessFactor() const noexcept;

		void SetRoughnessFactor(float factor) noexcept;

		[[nodiscard]]
		const float &GetAlphaThreshold() const noexcept;

		void SetAlphaThreshold(float alphaThreshold) noexcept;

		GraphicsApiMaterialInstance *GetInternalMaterial() override;

		MaterialResources &GetMaterialResources();

		MaterialConstants &GetMaterialConstants() noexcept;

		void SetMaterialConstants(const MaterialConstants& values);
		
	private:
		void BuildPipelines();

		MaterialConstants* m_materialConstants = nullptr;

		MaterialResources m_materialResources;

		EMaterialPass m_materialPass;

		std::unique_ptr<GraphicsApiMaterialInstance> m_internalMaterial;

		IRenderer *m_renderer;

		// Original material index
		// TODO: Check if we *actually* need this
		size_t m_materialIdx;
		
		EAlphaBlendMode m_alphaBlendMode;
	};

} // namespace Hush
