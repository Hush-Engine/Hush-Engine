#define VK_NO_PROTOTYPES

#include "VulkanPipelineBuilder.hpp"
#include "Logger.hpp"
#include "VkUtilsFactory.hpp"
#include <volk.h>

// NOLINTNEXTLINE (Initialization is handled on the clear method)
Hush::VulkanPipelineBuilder::VulkanPipelineBuilder()
{
    this->Clear();
}

void Hush::VulkanPipelineBuilder::Clear()
{
    // clear all of the structs we need back to 0 with their correct stype
    this->m_inputAssembly = {};
    this->m_inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;

    this->m_rasterizer = {};
    this->m_rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;

    this->m_colorBlendAttachment = {};

    this->m_multisampling = {};
    this->m_multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;

    this->m_pipelineLayout = {};

    this->m_depthStencil = {};
    this->m_depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;

    this->m_renderInfo = {};
    this->m_renderInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_RENDERING_CREATE_INFO;

    this->m_shaderStages.clear();
}

VkPipeline Hush::VulkanPipelineBuilder::Build(
    VkDevice device) // Code from https://vkguide.dev/docs/new_chapter_3/building_pipeline/
{
    // make viewport state from our stored viewport and scissor.
    // at the moment we wont support multiple viewports or scissors
    VkPipelineViewportStateCreateInfo viewportState = {};
    viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
    viewportState.pNext = nullptr;

    viewportState.viewportCount = 1;
    viewportState.scissorCount = 1;

    // setup dummy color blending. We arent using transparent objects yet
    // the blending is just "no blend", but we do write to the color attachment
    VkPipelineColorBlendStateCreateInfo colorBlending = {};
    colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
    colorBlending.pNext = nullptr;

    colorBlending.logicOpEnable = VK_FALSE;
    colorBlending.logicOp = VK_LOGIC_OP_COPY;
    colorBlending.attachmentCount = 1;
    colorBlending.pAttachments = &this->m_colorBlendAttachment;

    // completely clear VertexInputStateCreateInfo, as we have no need for it
    VkPipelineVertexInputStateCreateInfo vertexInputInfo = {};
    vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

    // build the actual pipeline
    // we now use all of the info structs we have been writing into into this one
    // to create the pipeline
    VkGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
    // connect the renderInfo to the pNext extension mechanism
    pipelineInfo.pNext = &this->m_renderInfo;

    pipelineInfo.stageCount = static_cast<uint32_t>(this->m_shaderStages.size());
    pipelineInfo.pStages = this->m_shaderStages.data();
    pipelineInfo.pVertexInputState = &vertexInputInfo;
    pipelineInfo.pInputAssemblyState = &this->m_inputAssembly;
    pipelineInfo.pViewportState = &viewportState;
    pipelineInfo.pRasterizationState = &this->m_rasterizer;
    pipelineInfo.pMultisampleState = &this->m_multisampling;
    pipelineInfo.pColorBlendState = &colorBlending;
    pipelineInfo.pDepthStencilState = &this->m_depthStencil;
    pipelineInfo.layout = this->m_pipelineLayout;

    VkDynamicState state[] = {VK_DYNAMIC_STATE_VIEWPORT, VK_DYNAMIC_STATE_SCISSOR};

    VkPipelineDynamicStateCreateInfo dynamicInfo = {};
    dynamicInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
    dynamicInfo.pDynamicStates = &state[0];
    dynamicInfo.dynamicStateCount = 2;

    pipelineInfo.pDynamicState = &dynamicInfo;

    // its easy to error out on create graphics pipeline, so we handle it a bit
    // better than the common VK_CHECK case
    VkPipeline newPipeline = nullptr;
    if (vkCreateGraphicsPipelines(device, nullptr, 1, &pipelineInfo, nullptr, &newPipeline) != VK_SUCCESS)
    {
        LogError("Failed to create the Vulkan Graphics Pipeline!");
        return nullptr;
    }
    return newPipeline;
}

Hush::VulkanPipelineBuilder &Hush::VulkanPipelineBuilder::SetShaders(VkShaderModule vertexShader,
                                                               VkShaderModule fragmentShader)
{
    //Create the information for the shaders to be added
    VkPipelineShaderStageCreateInfo vertexShaderInfo =
        VkUtilsFactory::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_VERTEX_BIT, vertexShader);
    VkPipelineShaderStageCreateInfo fragmentShaderInfo =
        VkUtilsFactory::PipelineShaderStageCreateInfo(VK_SHADER_STAGE_FRAGMENT_BIT, fragmentShader);
    //Clear the current stages and add both
    this->m_shaderStages.clear();
    this->m_shaderStages.push_back(vertexShaderInfo);

    this->m_shaderStages.push_back(fragmentShaderInfo);
    return *this;
}

Hush::VulkanPipelineBuilder &Hush::VulkanPipelineBuilder::SetInputTopology(VkPrimitiveTopology topology)
{
    this->m_inputAssembly.topology = topology;
    // TODO: Set this to true for indexed draws
    this->m_inputAssembly.primitiveRestartEnable = VK_FALSE;
    return *this;
}

Hush::VulkanPipelineBuilder &Hush::VulkanPipelineBuilder::SetPolygonMode(VkPolygonMode mode)
{
    this->m_rasterizer.polygonMode = mode;
    this->m_rasterizer.lineWidth = 1.0f;
    return *this;
}

Hush::VulkanPipelineBuilder &Hush::VulkanPipelineBuilder::SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace)
{
    this->m_rasterizer.cullMode = cullMode;
    this->m_rasterizer.frontFace = frontFace;
    return *this;
}

Hush::VulkanPipelineBuilder &Hush::VulkanPipelineBuilder::SetMultiSamplingNone()
{
    this->m_multisampling.sampleShadingEnable = VK_FALSE;
    // multisampling defaulted to no multisampling (1 sample per pixel)
    this->m_multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
    this->m_multisampling.minSampleShading = 1.0f;
    this->m_multisampling.pSampleMask = nullptr;
    // no alpha to coverage either
    this->m_multisampling.alphaToCoverageEnable = VK_FALSE;
    this->m_multisampling.alphaToOneEnable = VK_FALSE;
    return *this;
}

Hush::VulkanPipelineBuilder &Hush::VulkanPipelineBuilder::DisableBlending()
{
    // default write mask
    this->m_colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    // no blending
    this->m_colorBlendAttachment.blendEnable = VK_FALSE;
    return *this;
}

Hush::VulkanPipelineBuilder &Hush::VulkanPipelineBuilder::EnableBlendingAdditive()
{
    this->m_colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    this->m_colorBlendAttachment.blendEnable = VK_TRUE;
    this->m_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;
    this->m_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
    this->m_colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    this->m_colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    this->m_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    this->m_colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    return *this;
}

Hush::VulkanPipelineBuilder &Hush::VulkanPipelineBuilder::EnableBlendingAlphaBlend()
{
    this->m_colorBlendAttachment.colorWriteMask =
        VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
    this->m_colorBlendAttachment.blendEnable = VK_TRUE;
    this->m_colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_DST_ALPHA;
    this->m_colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_DST_ALPHA;
    this->m_colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
    this->m_colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
    this->m_colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
    this->m_colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
    return *this;
}

Hush::VulkanPipelineBuilder& Hush::VulkanPipelineBuilder::SetColorAttachmentFormat(VkFormat format)
{
    this->m_colorAttachmentformat = format;
    this->m_renderInfo.colorAttachmentCount = 1;
    this->m_renderInfo.pColorAttachmentFormats = &this->m_colorAttachmentformat;
    return *this;
}

Hush::VulkanPipelineBuilder &Hush::VulkanPipelineBuilder::SetDepthFormat(VkFormat format)
{
    this->m_renderInfo.depthAttachmentFormat = format;
    return *this;
}
bool Hush::VulkanHelper::LoadShaderModule(const std::string_view &filePath, VkDevice device,
                                          VkShaderModule *outShaderModule)
{
    // open the file. With cursor at the end
    std::ifstream file(filePath.data(), std::ios::ate | std::ios::binary);
    if (file.fail() || !file.is_open())
    {
        return false;
    }

    // find what the size of the file is by looking up the location of the cursor
    // because the cursor is at the end, it gives the size directly in bytes
    uint32_t fileSize = static_cast<uint32_t>(file.tellg());

    // spirv expects the buffer to be on uint32, so make sure to reserve a int
    // vector big enough for the entire file
    std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

    // put file cursor at beginning
    file.seekg(0);

    // load the entire file into the buffer
    auto *fileData =
        reinterpret_cast<char *>(buffer.data()); // We downsize this, but idk, this is how it expects us to use this
    file.read(fileData, fileSize);

    // now that the file is loaded into the buffer, we can close it
    file.close();

    // create a new shader module, using the buffer we loaded
    VkShaderModuleCreateInfo createInfo = {};
    createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
    createInfo.pNext = nullptr;

    // codeSize has to be in bytes, so multply the ints in the buffer by size of
    // int to know the real size of the buffer
    createInfo.codeSize = buffer.size() * sizeof(uint32_t);
    createInfo.pCode = buffer.data();

    // check that the creation goes well.
    VkShaderModule shaderModule = nullptr;
    if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
    {
        return false;
    }
    *outShaderModule = shaderModule;
    return true;
}
