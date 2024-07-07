/*! \file VkTypes.hpp
    \author Leonidas Gonzalez
    \date 2024-05-19
    \brief Type declarations for the Vulkan renderer
*/

#pragma once
#include "rendering/Vulkan/VulkanVertexBuffer.hpp"
#include "utils/Assertions.hpp"
#include <vulkan/vulkan.h>
#include <magic_enum.hpp>
#include <glm/glm.hpp>

// Stuff from vk_mem_alloc to avoid cyclical references
struct VmaAllocation_T;
using VmaAllocation = VmaAllocation_T *;
namespace Hush
{
    struct AllocatedImage
    {
        VkImage image;
        VkImageView imageView;
        VmaAllocation allocation;
        VkExtent3D imageExtent;
        VkFormat imageFormat;
    };

    enum class EMaterialPass : uint8_t
    {
        MainColor,
        Transparent,
        Other
    };

    struct GPUDrawPushConstants
    {
        glm::mat4 worldMatrix;
        VkDeviceAddress vertexBuffer;
    };

    struct GPUGLTFMaterial
    {
        glm::vec4 colorFactors;
        glm::vec4 metalRoughFactors;
        glm::vec4 extra[14];
    };

    struct GPUSceneData
    {
        glm::mat4 view;
        glm::mat4 projection;
        glm::mat4 viewProjection;
        glm::vec4 ambientColor;
        glm::vec4 sunlightDirection; // w for sun power
        glm::vec4 sunlightColor;
    };

    // holds the resources needed for a mesh
    struct GPUMeshBuffers {
        VulkanVertexBuffer indexBuffer;
        VulkanVertexBuffer vertexBuffer;
        VkDeviceAddress vertexBufferAddress{};
    };

    enum class EMaterialPass : uint8_t
    {
        MainColor,
        Transparent,
        Other
    };
}


#ifndef HUSH_VULKAN_IMPL
#define HUSH_VULKAN_IMPL
// NOLINTNEXTLINE
#define HUSH_VK_ASSERT(result, message)                                                                                \
    HUSH_ASSERT((result) == VkResult::VK_SUCCESS, "{} VK error code: {}", message, magic_enum::enum_name(result))
#endif
