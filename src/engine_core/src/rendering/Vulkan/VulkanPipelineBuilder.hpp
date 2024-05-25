/*! \file VulkanPipelineBuilder.hpp
    \author Kyn21kx
    \date 2024-05-06
    \brief Constructs the rendering pipeline for the vulkan renderer
*/

#pragma once
#include <vector>
#include <vulkan/vulkan.h>
namespace Hush
{
    class VulkanPipelineBuilder
    {
      public:
        VulkanPipelineBuilder();

        void Clear();

        VkPipeline Build(VkDevice device);

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
} // namespace Hush
