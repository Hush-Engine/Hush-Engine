#pragma once
#include "VulkanAllocatedBuffer.hpp"
#include <vulkan/vulkan_core.h>

namespace Hush
{

    // holds the resources needed for a mesh
    struct GPUMeshBuffers
    {
        GPUMeshBuffers() = default;

        GPUMeshBuffers(VulkanAllocatedBuffer indexBuffer, VulkanAllocatedBuffer vertexBuffer)
        {
            this->indexBuffer = indexBuffer;
            this->vertexBuffer = vertexBuffer;
            this->vertexBufferAddress = 0u;
        }

        VulkanAllocatedBuffer indexBuffer{};
        VulkanAllocatedBuffer vertexBuffer{};
        VkDeviceAddress vertexBufferAddress = 0u;
    };

} // namespace Hush
