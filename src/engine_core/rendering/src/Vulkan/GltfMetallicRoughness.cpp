
// NOTE: Keep volk at the top to avoid function redefinitions with Vulkan
#include <volk.h>
#include "GltfMetallicRoughness.hpp"
#include "GltfMetallicRoughness.hpp"
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
	layoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	layoutBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	layoutBuilder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	this->materialLayout = layoutBuilder.Build(device, VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

	// Create the mesh layouts
	VkDescriptorSetLayout layouts[] = {vkEngine->GetGpuSceneDataDescriptorLayout(), this->materialLayout};

	VkPipelineLayoutCreateInfo meshLayoutInfo = VkUtilsFactory::PipelineLayoutCreateInfo();
	meshLayoutInfo.setLayoutCount = 2;
	meshLayoutInfo.pSetLayouts = layouts;
	meshLayoutInfo.pPushConstantRanges = &matrixRange;
	meshLayoutInfo.pushConstantRangeCount = 1;

	VkPipelineLayout newLayout;
	VkResult rc = vkCreatePipelineLayout(device, &meshLayoutInfo, nullptr, &newLayout);
	HUSH_VK_ASSERT(rc, "Failed to create pipeline mesh pipeline layout!");

	this->opaquePipeline.layout = newLayout;
	this->transparentPipeline.layout = newLayout;

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
	pipelineBuilder.SetColorAttachmentFormat(vkEngine->GetDrawImage().imageFormat);
	pipelineBuilder.SetDepthFormat(vkEngine->GetDepthImage().imageFormat);

	// Create the opaque variant
	this->opaquePipeline.pipeline = pipelineBuilder.Build(device);

	// Create the transparent variant
	pipelineBuilder.EnableBlendingAdditive();

	pipelineBuilder.EnableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

	this->transparentPipeline.pipeline = pipelineBuilder.Build(device);

	// clean structures
	vkDestroyShaderModule(device, meshFragmentShader, nullptr);
	vkDestroyShaderModule(device, meshVertexShader, nullptr);
}

void Hush::GLTFMetallicRoughness::ClearResources(VkDevice device)
{
	(void)device;
}