#pragma once
#include <memory>
#define VK_NO_PROTOTYPES
#include <volk.h>

namespace Hush {
	class VulkanRenderer;
	class ShaderMaterial;

	class VulkanFullScreenPass {
	public:
		VulkanFullScreenPass() = default;

		VulkanFullScreenPass(VulkanRenderer* renderer, std::shared_ptr<ShaderMaterial> material);

		void RecordCommands(VkCommandBuffer cmd, VkDescriptorSet globalDescriptorSet);
		
		ShaderMaterial* GetMaterial();

	private:
		VulkanRenderer* m_renderer = nullptr;
		std::shared_ptr<ShaderMaterial> m_materialInstance;
	};
}
