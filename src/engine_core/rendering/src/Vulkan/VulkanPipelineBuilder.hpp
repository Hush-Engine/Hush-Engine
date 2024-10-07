/*! \file VulkanPipelineBuilder.hpp
    \author Kyn21kx
    \date 2024-05-06
    \brief Constructs the rendering pipeline for the vulkan renderer
*/

#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <string_view>
#include <iosfwd>
#include <fstream>

namespace Hush
{
    //TODO: REFACTOR
    class VulkanPipelineBuilder
    {
      public:
        VulkanPipelineBuilder(VkPipelineLayout pipelineLayout);

        void Clear();

        VkPipeline Build(VkDevice device);

        VulkanPipelineBuilder& SetShaders(VkShaderModule vertexShader, VkShaderModule fragmentShader);
        VulkanPipelineBuilder& SetInputTopology(VkPrimitiveTopology topology);
        VulkanPipelineBuilder& SetPolygonMode(VkPolygonMode mode);
        VulkanPipelineBuilder& SetCullMode(VkCullModeFlags cullMode, VkFrontFace frontFace);
        VulkanPipelineBuilder& SetMultiSamplingNone();
        VulkanPipelineBuilder& DisableBlending();
        VulkanPipelineBuilder& EnableBlendingAdditive();
        VulkanPipelineBuilder& EnableBlendingAlphaBlend();
        VulkanPipelineBuilder& DisableDepthTest();

        VulkanPipelineBuilder& SetColorAttachmentFormat(VkFormat format);
        VulkanPipelineBuilder& SetDepthFormat(VkFormat format);

      private:
        std::vector<VkPipelineShaderStageCreateInfo> m_shaderStages;

        VkPipelineInputAssemblyStateCreateInfo m_inputAssembly;
        VkPipelineRasterizationStateCreateInfo m_rasterizer;
        VkPipelineColorBlendAttachmentState m_colorBlendAttachment;
        VkPipelineMultisampleStateCreateInfo m_multisampling;
        VkPipelineLayout m_pipelineLayout = nullptr;
        VkPipelineDepthStencilStateCreateInfo m_depthStencil;
        VkPipelineRenderingCreateInfo m_renderInfo;
        VkFormat m_colorAttachmentformat;
    };

    class VulkanHelper final
    {
      public:
        static bool LoadShaderModule(const std::string_view &filePath, VkDevice device,
                                     VkShaderModule *outShaderModule);
    };

} // namespace Hush
