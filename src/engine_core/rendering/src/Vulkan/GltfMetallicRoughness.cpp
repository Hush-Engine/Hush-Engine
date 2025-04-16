
// NOTE: Keep volk at the top to avoid function redefinitions with Vulkan
#include <cstdint>
#include <volk.h>
#include <vulkan/vulkan_core.h>
#include "GltfMetallicRoughness.hpp"
#include "Shared/MaterialOptions.hpp"
#include "VulkanRenderer.hpp"
#include "VulkanPipelineBuilder.hpp"
#include "VkUtilsFactory.hpp"
#include "VkMaterialInstance.hpp"

void Hush::GLTFMetallicRoughness::BuildPipelines(IRenderer *engine, const std::string_view &fragmentShaderPath,
												 const std::string_view &vertexShaderPath)
{
	auto *vkEngine = static_cast<VulkanRenderer *>(engine);
	VkShaderModule meshFragmentShader;
	VkDevice device = vkEngine->GetVulkanDevice();

	if (!VulkanHelper::LoadShaderModule(fragmentShaderPath, device, &meshFragmentShader))
	{
		LogError("Error when building the triangle fragment shader module");
	}

	VkShaderModule meshVertexShader;
	if (!VulkanHelper::LoadShaderModule(vertexShaderPath, device, &meshVertexShader))
	{
		LogError("Error when building the triangle vertex shader module");
	}

	VkPushConstantRange matrixRange{};
	matrixRange.offset = 0;
	matrixRange.size = sizeof(GPUDrawPushConstants);
	matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

	// Create the material layout
	DescriptorLayoutBuilder layoutBuilder{};

	constexpr uint32_t uniformBufferBinding = 0;
	constexpr uint32_t albedoBinding = 1;
	constexpr uint32_t metallicBinding = 2;
	constexpr uint32_t normalBinding = 3;
	constexpr uint32_t emissionBinding = 4;

	layoutBuilder.AddBinding(uniformBufferBinding, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	layoutBuilder.AddBinding(albedoBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	layoutBuilder.AddBinding(metallicBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	layoutBuilder.AddBinding(normalBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	layoutBuilder.AddBinding(emissionBinding, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	this->m_materialLayout = layoutBuilder.Build(device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	// Create the mesh layouts
	VkDescriptorSetLayout layouts[] = {vkEngine->GetGpuSceneDataDescriptorLayout(), this->m_materialLayout};

	VkPipelineLayoutCreateInfo meshLayoutInfo = VkUtilsFactory::PipelineLayoutCreateInfo();
	meshLayoutInfo.setLayoutCount = 2;
	meshLayoutInfo.pSetLayouts = layouts;
	meshLayoutInfo.pPushConstantRanges = &matrixRange;
	meshLayoutInfo.pushConstantRangeCount = 1;

	VkPipelineLayout newLayout;
	VkResult rc = vkCreatePipelineLayout(device, &meshLayoutInfo, nullptr, &newLayout);
	HUSH_VK_ASSERT(rc, "Failed to create pipeline mesh pipeline layout!");

	this->m_opaquePipeline.layout = newLayout;
	this->m_transparentPipeline.layout = newLayout;

	// build the stage-create-info for both vertex and fragment stages. This lets
	// the pipeline know the shader modules per stage
	VulkanPipelineBuilder pipelineBuilder(newLayout);
	pipelineBuilder.SetShaders(meshVertexShader, meshFragmentShader);
	pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);
	pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);
	pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);
	pipelineBuilder.SetMultiSamplingNone();
	pipelineBuilder.DisableBlending();
	pipelineBuilder.EnableDepthTest(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

	// Render format
	pipelineBuilder.SetColorAttachmentFormat(static_cast<VkFormat>(vkEngine->GetDrawImage().imageFormat));
	pipelineBuilder.SetDepthFormat(static_cast<VkFormat>(vkEngine->GetDepthImage().imageFormat));

	// Create the opaque variant
	this->m_opaquePipeline.pipeline = pipelineBuilder.Build(device);

	// Create the transparent variant
	pipelineBuilder.EnableBlendingAdditive();

	pipelineBuilder.EnableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

	this->m_transparentPipeline.pipeline = pipelineBuilder.Build(device);

	// clean structures
	vkDestroyShaderModule(device, meshFragmentShader, nullptr);
	vkDestroyShaderModule(device, meshVertexShader, nullptr);
}

void Hush::GLTFMetallicRoughness::ClearResources(VkDevice device)
{
	(void)device;
}

Hush::EAlphaBlendMode Hush::GLTFMetallicRoughness::GetAlphaBlendMode() const noexcept
{
	return EAlphaBlendMode::None;
}

Hush::ECullMode Hush::GLTFMetallicRoughness::GetCullMode() const noexcept
{
	return ECullMode::None;
}

void Hush::GLTFMetallicRoughness::SetCullMode(ECullMode cullMode)
{
	(void)cullMode;
}

void Hush::GLTFMetallicRoughness::SetAlphaBlendMode(EAlphaBlendMode blendMode) noexcept
{
	(void)blendMode;
}
