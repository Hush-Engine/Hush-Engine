#include "GpuAllocatedBuffer.hpp"
#include "Assertions.hpp"
#include "BitwiseUtils.hpp"
#include <cstddef>
#include <cstdint>
#include <memory>

#if defined(HUSH_VULKAN_IMPL)
#include <vulkan/vulkan.h>
#include "Vulkan/VkTypes.hpp"
#include "Vulkan/vk_mem_alloc.hpp"
#include <vulkan/vulkan_core.h>

struct Hush::GpuAllocatedBuffer::OpaqueAllocationData
{
	VmaAllocation allocation;
	VmaAllocationInfo allocInfo;
};

#endif

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
	if (Hush::Bitwise::HasCompositeFlag(cmpBufferUsage,
										static_cast<uint32_t>(Hush::GpuAllocatedBuffer::EBufferUsage::VertexBuffer)))
	{
		flags |= VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
	}
	if (Hush::Bitwise::HasCompositeFlag(cmpBufferUsage,
										static_cast<uint32_t>(Hush::GpuAllocatedBuffer::EBufferUsage::IndexBuffer)))
	{
		flags |= VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
	}
	if (Hush::Bitwise::HasCompositeFlag(cmpBufferUsage,
										static_cast<uint32_t>(Hush::GpuAllocatedBuffer::EBufferUsage::UniformBuffer)))
	{
		flags |= VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
	}
	if (Hush::Bitwise::HasCompositeFlag(cmpBufferUsage,
										static_cast<uint32_t>(Hush::GpuAllocatedBuffer::EBufferUsage::TransferSrc)))
	{
		flags |= VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
	}
	if (Hush::Bitwise::HasCompositeFlag(cmpBufferUsage,
										static_cast<uint32_t>(Hush::GpuAllocatedBuffer::EBufferUsage::TransferDst)))
	{
		flags |= VK_BUFFER_USAGE_TRANSFER_DST_BIT;
	}
	if (Hush::Bitwise::HasCompositeFlag(cmpBufferUsage,
										static_cast<uint32_t>(Hush::GpuAllocatedBuffer::EBufferUsage::StorageBuffer)))
	{
		flags |= VK_BUFFER_USAGE_STORAGE_BUFFER_BIT;
	}
	if (Hush::Bitwise::HasCompositeFlag(
			cmpBufferUsage, static_cast<uint32_t>(Hush::GpuAllocatedBuffer::EBufferUsage::ShaderDeviceAddress)))
	{
		flags |= VK_BUFFER_USAGE_SHADER_DEVICE_ADDRESS_BIT;
	}
	return flags;
}

Hush::GpuAllocatedBuffer::GpuAllocatedBuffer(size_t size, EBufferUsage usage, EMemoryUsage memoryUsage, void *allocator)
	: m_size(size),
	  m_capacity(size)
{
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
	// It's important to reinterpret instead of using just a normal static cast here
	// or we will bind to a temporary address
	auto **vkBuffer = reinterpret_cast<VkBuffer *>(&this->m_buffer);
	auto **vmaAllocation = reinterpret_cast<VmaAllocation *>(&this->m_allocation);
	VmaAllocationInfo info;
	HUSH_VK_ASSERT(vmaCreateBuffer(static_cast<VmaAllocator>(allocator), &bufferInfo, &vmaallocInfo, vkBuffer,
								   vmaAllocation, &info),
				   "Buffer allocation failed!");
	this->m_offset = info.offset;
	this->m_mappedData = info.pMappedData;
	this->m_userData = info.pUserData;
}

void Hush::GpuAllocatedBuffer::Dispose(void *allocator)
{
	if (this->m_buffer == nullptr)
	{
		return;
	}
	// NOLINTNEXTLINE
	vmaDestroyBuffer(reinterpret_cast<VmaAllocator>(allocator), reinterpret_cast<VkBuffer>(this->m_buffer),
					 reinterpret_cast<VmaAllocation>(this->m_allocation));
}

size_t Hush::GpuAllocatedBuffer::GetSize() const noexcept
{
	return this->m_size;
}

void *Hush::GpuAllocatedBuffer::GetBuffer()
{
	return this->m_buffer;
}

void *Hush::GpuAllocatedBuffer::GetMappedData()
{
	return this->m_mappedData;
}
