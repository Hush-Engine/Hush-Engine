#include "GpuAllocatedBuffer.hpp"
#include "Assertions.hpp"
#include "BitwiseUtils.hpp"
#include "Vulkan/VkTypes.hpp"
#include "Vulkan/vk_mem_alloc.hpp"
#include <cstdint>
#include <memory>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>

VmaMemoryUsage HushMemoryUsageToVmaUsage(Hush::GpuAllocatedBuffer::EMemoryUsage memoryUsage)
{
	switch (memoryUsage)
	{
	case Hush::GpuAllocatedBuffer::EMemoryUsage::GpuOnly:
		return VMA_MEMORY_USAGE_GPU_ONLY;
	case Hush::GpuAllocatedBuffer::EMemoryUsage::CpuOnly:
		return VMA_MEMORY_USAGE_CPU_ONLY;
	case Hush::GpuAllocatedBuffer::EMemoryUsage::CpuToGpu:
		return VMA_MEMORY_USAGE_CPU_TO_GPU;
	case Hush::GpuAllocatedBuffer::EMemoryUsage::GpuToCpu:
		return VMA_MEMORY_USAGE_GPU_TO_CPU;
		break;
	default:
		HUSH_ASSERT(false, "Unrecognized buffer memory usage: {}", static_cast<uint32_t>(memoryUsage));
		return VMA_MEMORY_USAGE_UNKNOWN;
	}
}

VkBufferUsageFlags HushBufferUsageToVkBufferUsage(Hush::GpuAllocatedBuffer::EBufferUsage bufferUsage)
{

	VkBufferUsageFlags flags = 0;
	auto cmpBufferUsage = static_cast<uint32_t>(bufferUsage);
	if (Hush::Bitwise::HasCompositeFlag(cmpBufferUsage, static_cast<uint32_t>(Hush::GpuAllocatedBuffer::EBufferUsage::VertexBuffer)))
	{
		flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	}
	if (Hush::Bitwise::HasCompositeFlag(cmpBufferUsage, static_cast<uint32_t>(Hush::GpuAllocatedBuffer::EBufferUsage::IndexBuffer)))
	{
		flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	}
	if (Hush::Bitwise::HasCompositeFlag(cmpBufferUsage, static_cast<uint32_t>(Hush::GpuAllocatedBuffer::EBufferUsage::UniformBuffer)))
	{
		flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	}
	if (Hush::Bitwise::HasCompositeFlag(cmpBufferUsage, static_cast<uint32_t>(Hush::GpuAllocatedBuffer::EBufferUsage::TransferSrc)))
	{
		flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	}
	if (Hush::Bitwise::HasCompositeFlag(cmpBufferUsage, static_cast<uint32_t>(Hush::GpuAllocatedBuffer::EBufferUsage::TransferDst)))
	{
		flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}
	if (Hush::Bitwise::HasCompositeFlag(cmpBufferUsage, static_cast<uint32_t>(Hush::GpuAllocatedBuffer::EBufferUsage::StorageBuffer)))
	{
		flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	}
	if (Hush::Bitwise::HasCompositeFlag(cmpBufferUsage, static_cast<uint32_t>(Hush::GpuAllocatedBuffer::EBufferUsage::ShaderDeviceAddress)))
	{
		flags |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	}
	return flags;
}

Hush::GpuAllocatedBuffer::GpuAllocatedBuffer(uint32_t size, EBufferUsage usage, EMemoryUsage memoryUsage,
											 GraphicsAllocator allocator)
	: m_size(size),
	  m_capacity(size)
{
	// TODO: Get rid of this pointer for a value type
	this->m_allocInfo = std::make_shared<GpuAllocationInfo>();
	// allocate buffer
	VkBufferCreateInfo bufferInfo = {};
	bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
	bufferInfo.pNext = nullptr;
	bufferInfo.size = size;

	bufferInfo.usage = HushBufferUsageToVkBufferUsage(usage);

	VmaAllocationCreateInfo vmaallocInfo = {};
	vmaallocInfo.usage = HushMemoryUsageToVmaUsage(memoryUsage);
	vmaallocInfo.flags = VMA_ALLOCATION_CREATE_MAPPED_BIT;

	// allocate the buffer
	HUSH_VK_ASSERT(vmaCreateBuffer(allocator, &bufferInfo, &vmaallocInfo, &this->m_buffer, &this->m_allocation,
								   this->m_allocInfo.get()),
				   "Buffer allocation failed!");
}

void Hush::GpuAllocatedBuffer::Dispose(GraphicsAllocator allocator)
{
	vmaDestroyBuffer(allocator, this->m_buffer, this->m_allocation);
}

uint32_t Hush::GpuAllocatedBuffer::GetSize() const noexcept
{
	return this->m_size;
}

GpuAllocation Hush::GpuAllocatedBuffer::GetAllocation()
{
	return this->m_allocation;
}

GpuBuffer Hush::GpuAllocatedBuffer::GetBuffer()
{
	return this->m_buffer;
}

GpuAllocationInfo *Hush::GpuAllocatedBuffer::GetAllocationInfo()
{
	return this->m_allocInfo.get();
}
