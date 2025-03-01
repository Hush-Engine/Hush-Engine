#include "VulkanFullScreenPass.hpp"
#include "Shared/ShaderMaterial.hpp"
#include "VulkanRenderer.hpp"
#include "VulkanPipelineBuilder.hpp"

namespace Hush {
	struct OpaqueMaterialData {
		VkMaterialPipeline pipeline;
		VkDescriptorSetLayout descriptorLayout;
		DescriptorWriter writer;
		VkBufferCreateInfo uniformBufferCreateInfo;
	};
}

Hush::VulkanFullScreenPass::VulkanFullScreenPass(VulkanRenderer* renderer, std::shared_ptr<ShaderMaterial> material)
{
	this->m_renderer = renderer;
	this->m_materialInstance = material;
}

void Hush::VulkanFullScreenPass::RecordCommands(VkCommandBuffer cmd, VkDescriptorSet globalDescriptorSet)
{
	(void)globalDescriptorSet;
	OpaqueMaterialData* matData = this->m_materialInstance->GetMaterialData();
	VkPipeline pipeline = matData->pipeline.pipeline;
	VkDescriptorSet descSet = this->m_materialInstance->GetInternalMaterial().materialSet;
	printf("Global set: %p\tMaterial set: %p\n", globalDescriptorSet, descSet);
	// Bind pipeline
	vkCmdBindPipeline(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, pipeline);
	vkCmdBindDescriptorSets(cmd, VK_PIPELINE_BIND_POINT_GRAPHICS, matData->pipeline.layout, 0, 1, &descSet, 0, nullptr);
	// Draw full-screen quad
	vkCmdDraw(cmd, 6, 1, 0, 0);
}

Hush::ShaderMaterial* Hush::VulkanFullScreenPass::GetMaterial()
{
	return this->m_materialInstance.get();
}

