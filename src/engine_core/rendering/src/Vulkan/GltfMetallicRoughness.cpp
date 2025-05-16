
// NOTE: Keep volk at the top to avoid function redefinitions with Vulkan
#include <cstdint>
#include <glm/ext/vector_float3.hpp>
#include <string_view>
#include <volk.h>
#include <vulkan/vulkan_core.h>
#include "GltfMetallicRoughness.hpp"
#include "Assertions.hpp"
#include "Shared/MaterialOptions.hpp"
#include "Shared/MaterialPass.hpp"
#include "VulkanRenderer.hpp"
#include "VulkanPipelineBuilder.hpp"
#include "VkUtilsFactory.hpp"
#include "VkMaterialInstance.hpp"

constexpr std::string_view FRAGMENT_SHADER_PATH = R"(C:\Users\nefes\Personal\Hush-Engine\res\mesh.frag.spv)";
constexpr std::string_view VERTEX_SHADER_PATH = R"(C:\Users\nefes\Personal\Hush-Engine\res\mesh.vert.spv)";

void Hush::GLTFMetallicRoughness::Init(IRenderer *renderer, GpuAllocatedBuffer materialBuffer, size_t materialIdx, uint32_t dataBufferOffset)
{
	this->m_renderer = renderer;
	this->m_materialResources.gpuDataBuffer = materialBuffer;
	this->m_materialResources.dataBufferOffset = dataBufferOffset;
	this->m_materialConstants = reinterpret_cast<MaterialConstants*>(this->m_materialResources.gpuDataBuffer.GetMappedData());
	// We have to manually set the options here lol
	this->m_materialConstants->options = 0;
	this->m_materialIdx = materialIdx;
	this->BuildPipelines();
}

void Hush::GLTFMetallicRoughness::BuildPipelines()
{
	auto *vkEngine = dynamic_cast<VulkanRenderer *>(this->m_renderer);
	VkShaderModule meshFragmentShader = nullptr;
	VkDevice device = vkEngine->GetVulkanDevice();

	if (!vkEngine->GetShaderModuleLoader().LoadShaderModule(FRAGMENT_SHADER_PATH, device, &meshFragmentShader))
	{
		LogError("Error when building the triangle fragment shader module");
	}

	VkShaderModule meshVertexShader = nullptr;
	if (!vkEngine->GetShaderModuleLoader().LoadShaderModule(VERTEX_SHADER_PATH, device, &meshVertexShader))
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

	VkPipelineLayout newLayout = nullptr;
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

	pipelineBuilder.SetAlphaBlendMode(EAlphaBlendMode::OneMinusSrcAlpha);
	// Create the transparent variant
	// pipelineBuilder.EnableBlendingAdditive();

	pipelineBuilder.EnableDepthTest(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

	this->m_transparentPipeline.pipeline = pipelineBuilder.Build(device);

	// NOTE: I don't think we need to delete this shader module, as the PBR shader is the ONE shader that will be there
	// all the time for all objects (by default) vkDestroyShaderModule(device, meshFragmentShader, nullptr);
	// vkDestroyShaderModule(device, meshVertexShader, nullptr);
}

void Hush::GLTFMetallicRoughness::GenerateMaterialInstance(DescriptorAllocatorGrowable *descriptorAllocator)
{
	auto *rendererImpl = dynamic_cast<VulkanRenderer *>(this->m_renderer);
	VkDevice device = rendererImpl->GetVulkanDevice();

	this->m_internalMaterial = std::make_unique<GraphicsApiMaterialInstance>();

	this->m_internalMaterial->passType = this->m_materialPass;
	switch (this->m_internalMaterial->passType)
	{
	case Hush::EMaterialPass::Mask:

	case Hush::EMaterialPass::MainColor:
		this->m_internalMaterial->pipeline = &this->m_opaquePipeline;
		break;
	case Hush::EMaterialPass::Transparent:
		this->m_internalMaterial->pipeline = &this->m_transparentPipeline;
		break;
	default:
		HUSH_ASSERT(false, "Unkown material pass: {}", magic_enum::enum_name(this->m_internalMaterial->passType));
		break;
	}

	// Not initialized material layout here from VkLoader
	this->m_internalMaterial->materialSet = descriptorAllocator->Allocate(device, this->m_materialLayout);

	auto *rawDataBuffer = reinterpret_cast<VkBuffer>(this->m_materialResources.gpuDataBuffer.GetBuffer());

	// Write the resources to the buffer
	writer.Clear();
	writer.WriteBuffer(0, rawDataBuffer, sizeof(MaterialConstants), this->m_materialResources.dataBufferOffset,
					   VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
	writer.WriteImage(1, this->m_materialResources.colorImage.imageView, this->m_materialResources.colorSampler,
					  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
	writer.WriteImage(2, this->m_materialResources.metalRoughImage.imageView,
					  this->m_materialResources.metalRoughSampler, VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
					  VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	writer.WriteImage(3, this->m_materialResources.normalImage.imageView, this->m_materialResources.normalSampler,
					  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	writer.WriteImage(4, this->m_materialResources.emissiveImage.imageView, this->m_materialResources.emissiveSampler,
					  VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

	writer.UpdateSet(device, this->m_internalMaterial->materialSet);
}

void Hush::GLTFMetallicRoughness::ClearResources(VkDevice device)
{
	(void)device;
}

Hush::EAlphaBlendMode Hush::GLTFMetallicRoughness::GetAlphaBlendMode() const noexcept
{
	return this->m_alphaBlendMode;
}

Hush::ECullMode Hush::GLTFMetallicRoughness::GetCullMode() const noexcept
{
	return ECullMode::None;
}

Hush::EMaterialPass Hush::GLTFMetallicRoughness::GetMaterialPass() const noexcept
{
	return this->m_materialPass;
}

void Hush::GLTFMetallicRoughness::SetMaterialPass(EMaterialPass pass)
{
	this->m_materialPass = pass;
}

void Hush::GLTFMetallicRoughness::SetCullMode(ECullMode cullMode)
{
	(void)cullMode;
}

void Hush::GLTFMetallicRoughness::SetAlphaBlendMode(EAlphaBlendMode blendMode) noexcept
{
	this->m_alphaBlendMode = blendMode;
}

const glm::vec4 &Hush::GLTFMetallicRoughness::GetAlbedo() const noexcept
{
	return this->m_materialConstants->colorFactors;
}

glm::vec4 &Hush::GLTFMetallicRoughness::GetAlbedo() noexcept
{
	return this->m_materialConstants->colorFactors;
}

void Hush::GLTFMetallicRoughness::SetAlbedo(const glm::vec4 &color) noexcept
{
	this->m_materialConstants->colorFactors = color;
}

const glm::vec3 &Hush::GLTFMetallicRoughness::GetEmissionColor() const noexcept
{
	// Reinterpret the vec4 into a vec3, we'll be leaving out the w component in the memory span, this is worth it since
	// we're returning a reference
	return *reinterpret_cast<const glm::vec3 *>(&this->m_materialConstants->emissionFactors);
}

glm::vec3 &Hush::GLTFMetallicRoughness::GetEmissionColor() noexcept
{
	// Reinterpret the vec4 into a vec3, we'll be leaving out the w component in the memory span, this is worth it since
	// we're returning a reference
	return *reinterpret_cast<glm::vec3 *>(&this->m_materialConstants->emissionFactors);
}

void Hush::GLTFMetallicRoughness::SetEmissionColor(const glm::vec3 &color) noexcept
{
	// TODO: Optimize this
	this->m_materialConstants->emissionFactors.x = color.x;
	this->m_materialConstants->emissionFactors.y = color.y;
	this->m_materialConstants->emissionFactors.z = color.z;
}

const float &Hush::GLTFMetallicRoughness::EmissionFactor() const noexcept
{
	return this->m_materialConstants->emissionFactors.w;
}

void Hush::GLTFMetallicRoughness::SetEmissionFactor(float emissionFactor) noexcept
{
	this->m_materialConstants->emissionFactors.w = emissionFactor;
}

const float &Hush::GLTFMetallicRoughness::GetMetallicFactor() const noexcept
{
	return this->m_materialConstants->metalRoughFactors.x;
}

void Hush::GLTFMetallicRoughness::SetMetallicFactor(float factor) noexcept
{
	this->m_materialConstants->metalRoughFactors.x = factor;
}

const float &Hush::GLTFMetallicRoughness::GetRoughnessFactor() const noexcept
{
	return this->m_materialConstants->metalRoughFactors.y;
}

void Hush::GLTFMetallicRoughness::SetRoughnessFactor(float factor) noexcept
{
	this->m_materialConstants->metalRoughFactors.y = factor;
}

const float &Hush::GLTFMetallicRoughness::GetAlphaThreshold() const noexcept
{
	return this->m_materialConstants->alphaThreshold;
}

void Hush::GLTFMetallicRoughness::SetAlphaThreshold(float alphaThreshold) noexcept
{
	this->m_materialConstants->alphaThreshold = alphaThreshold;
}

Hush::GraphicsApiMaterialInstance *Hush::GLTFMetallicRoughness::GetInternalMaterial()
{
	return this->m_internalMaterial.get();
}

Hush::GLTFMetallicRoughness::MaterialConstants &Hush::GLTFMetallicRoughness::GetMaterialConstants() noexcept
{
	HUSH_ASSERT(this->m_materialConstants != nullptr, "No data in material, did you forget to call GenerateMaterialInstance?");
	return this->m_materialConstants[this->m_materialIdx]; // Uuuuh, yeah, that works I guess
}


void Hush::GLTFMetallicRoughness::SetMaterialConstants(const MaterialConstants& values) {
	HUSH_ASSERT(this->m_materialConstants != nullptr, "No data in material, did you forget to call GenerateMaterialInstance?");
	this->m_materialConstants[this->m_materialIdx] = values;
}


Hush::GLTFMetallicRoughness::MaterialResources &Hush::GLTFMetallicRoughness::GetMaterialResources()
{
	return this->m_materialResources;
}
