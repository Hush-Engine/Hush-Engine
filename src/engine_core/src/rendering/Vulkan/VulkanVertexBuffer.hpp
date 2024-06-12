/*! \file VulkanVertexBuffer.hpp
    \author Kyn21kx
    \date 2024-05-28
    \brief Vulkan implementation of the Vertex Buffer class
*/

#pragma once
#include <vulkan/vulkan.h>
#include "VkTypes.hpp"
#include "vk_mem_alloc.hpp"

namespace Hush
{
    class VulkanVertexBuffer final
    {
    public:
        VulkanVertexBuffer(uint32_t size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocator allocator);

        [[nodiscard]] uint32_t GetSize() const noexcept;

        [[nodiscard]] VmaAllocation GetAllocation() noexcept;

        [[nodiscard]] VkBuffer GetBuffer() noexcept;

    private:
        VkBuffer m_buffer = nullptr;

        VmaAllocation m_allocation = nullptr;
        VmaAllocationInfo m_allocInfo{};

        /// @brief The size of the current data in the buffer, must be <= m_capacity
        uint32_t m_size = 0;
        /// @brief The initial size of the buffer's data, and therefore, the max size it accepts
        uint32_t m_capacity = 0;
    };
}