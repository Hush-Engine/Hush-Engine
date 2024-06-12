#include "GltfMetallicRoughness.hpp"
#include "VulkanRenderer.hpp"
#include "VkOperations.hpp"
#include "VkTypes.hpp"
#include "VkUtilsFactory.hpp"
#include "VulkanPipelineBuilder.hpp"

void Hush::GLTFMetallicRoughness::BuildPipelines(IRenderer *engine)
{
    // #TODO: Load shaders dynamically
    auto *renderer = dynamic_cast<VulkanRenderer*>(engine);
    VkShaderModule meshFragShader = nullptr;
    bool loadedShader = VkOperations::LoadShaderModule("../../shaders/mesh.frag.spv", renderer->GetVulkanDevice(), &meshFragShader);
    HUSH_ASSERT(loadedShader, "Error when building the triangle fragment shader module");

    VkShaderModule meshVertexShader = nullptr;
    loadedShader = VkOperations::LoadShaderModule("../../shaders/mesh.vert.spv", renderer->GetVulkanDevice(), &meshVertexShader);
    HUSH_ASSERT(loadedShader, "Error when building the triangle vertex shader module");

    VkPushConstantRange matrixRange{};
    matrixRange.offset = 0;
    matrixRange.size = sizeof(GPUDrawPushConstants);
    matrixRange.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

    DescriptorLayoutBuilder layoutBuilder;
    layoutBuilder.AddBinding(0, VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER);
    layoutBuilder.AddBinding(1, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);
    layoutBuilder.AddBinding(2, VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER);

    materialLayout = layoutBuilder.Build(renderer->GetVulkanDevice(), VK_SHADER_STAGE_VERTEX_BIT | VK_SHADER_STAGE_FRAGMENT_BIT);

    VkDescriptorSetLayout layouts[] = {renderer->_gpuSceneDataDescriptorLayout, materialLayout};

    VkPipelineLayoutCreateInfo mesh_layout_info = VkUtilsFactory::PipelineLayoutCreateInfo();
    mesh_layout_info.setLayoutCount = 2;
    mesh_layout_info.pSetLayouts = layouts;
    mesh_layout_info.pPushConstantRanges = &matrixRange;
    mesh_layout_info.pushConstantRangeCount = 1;

    VkPipelineLayout newLayout = nullptr;
    HUSH_VK_ASSERT(vkCreatePipelineLayout(renderer->GetVulkanDevice(), &mesh_layout_info, nullptr, &newLayout), "Failed to create pipeline layout!");

    opaquePipeline.layout = newLayout;
    transparentPipeline.layout = newLayout;

    // build the stage-create-info for both vertex and fragment stages. This lets
    // the pipeline know the shader modules per stage
    VulkanPipelineBuilder pipelineBuilder;

    pipelineBuilder.SetShaders(meshVertexShader, meshFragShader);

    pipelineBuilder.SetInputTopology(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST);

    pipelineBuilder.SetPolygonMode(VK_POLYGON_MODE_FILL);

    pipelineBuilder.SetCullMode(VK_CULL_MODE_NONE, VK_FRONT_FACE_CLOCKWISE);

    pipelineBuilder.SetMultiSamplingNone();

    pipelineBuilder.DisableBlending();

    pipelineBuilder.EnableDepthTesting(true, VK_COMPARE_OP_GREATER_OR_EQUAL);

    // render format
    pipelineBuilder.SetColorAttachmentFormat(renderer->GetDrawImage().imageFormat);
    pipelineBuilder.SetDepthFormat(renderer->dep.imageFormat);

    // use the triangle layout we created
    pipelineBuilder.SetPipelineLayout(newLayout);

    // finally build the pipeline
    opaquePipeline.pipeline = pipelineBuilder.Build(renderer->GetVulkanDevice());

    // create the transparent variant
    pipelineBuilder.EnableBlendingAdditive();

    pipelineBuilder.EnableDepthTesting(false, VK_COMPARE_OP_GREATER_OR_EQUAL);

    transparentPipeline.pipeline = pipelineBuilder.Build();

    vkDestroyShaderModule(renderer->GetVulkanDevice(), meshFragShader, nullptr);
    vkDestroyShaderModule(renderer->GetVulkanDevice(), meshVertexShader, nullptr);
}