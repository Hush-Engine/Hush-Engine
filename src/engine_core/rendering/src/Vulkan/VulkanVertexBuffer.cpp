#include "VulkanVertexBuffer.hpp"
#include "VkTypes.hpp"

Hush::VulkanVertexBuffer::VulkanVertexBuffer(uint32_t size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocator allocator)
{
    // allocate buffer
    VkBufferCreateInfo bufferInfo = {};
    bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
    bufferInfo.pNext = nullptr;
    bufferInfo.size = size;

    bufferInfo.usage = usage;

    VmaAllocationCreateInfo vmaallocInfo = {};
    vmaallocInfo.usage = memoryUsage;
    vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

    // allocate the buffer
    HUSH_VK_ASSERT(vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo, &this->m_buffer, &this->m_allocation,
                                   &this->m_allocInfo),
                   "Vertex buffer allocation failed!");
}

uint32_t Hush::VulkanVertexBuffer::GetSize() const noexcept
{
    return this->m_size;
}
