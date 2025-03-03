#include "VulkanAllocatedBuffer.hpp"
#include "VkTypes.hpp"
#include "vk_mem_alloc.hpp"

Hush::VulkanAllocatedBuffer::VulkanAllocatedBuffer(uint32_t size, VkBufferUsageFlags usage, VmaMemoryUsage memoryUsage, VmaAllocator allocator)
{
    // allocate buffer
    VkBufferCreateInfo bufferInfo = {};
    this->m_size = size;
    this->m_capacity = size;
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
                   "Buffer allocation failed!");
    this->m_allocatorRef = allocator;
}

void Hush::VulkanAllocatedBuffer::Dispose(VmaAllocator allocator) const
{
    vmaDestroyBuffer(allocator, this->m_buffer, this->m_allocation);
}

uint32_t Hush::VulkanAllocatedBuffer::GetSize() const noexcept
{
    return this->m_size;
}

VmaAllocation Hush::VulkanAllocatedBuffer::GetAllocation()
{
    return this->m_allocation;
}

VkBuffer Hush::VulkanAllocatedBuffer::GetBuffer()
{
    return this->m_buffer;
}

VmaAllocationInfo& Hush::VulkanAllocatedBuffer::GetAllocationInfo() noexcept
{
    return this->m_allocInfo;
}
