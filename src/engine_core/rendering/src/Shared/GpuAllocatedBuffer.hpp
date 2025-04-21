#pragma once

#include "Shared/GpuAllocatedImage.hpp"
#include <cstdint>
#include <memory>

#if defined(HUSH_VULKAN_IMPL)
		struct VmaAllocator_T;
		struct VmaAllocation_T;
		struct VmaAllocationInfo;
		struct VkBuffer_T;

		using GraphicsAllocator = VmaAllocator_T *;
		using GpuAllocation = VmaAllocation_T *;
		using GpuAllocationInfo = VmaAllocationInfo;
		using GpuBuffer = VkBuffer_T *;
#endif

namespace Hush
{

	class GpuAllocatedBuffer final
	{
	public:
		enum class EBufferUsage : uint32_t
		{
			None = 0x0,
			VertexBuffer = 0b00000001,
			IndexBuffer = 0b00000010,
			UniformBuffer = 0b00000100,
			TransferSrc = 0b00001000,
			TransferDst = 0b00010000,
			StorageBuffer = 0b00100000,
			ShaderDeviceAddress = 0b01000000
		};

		enum class EMemoryUsage : uint32_t
		{
			GpuOnly,
			CpuOnly,
			CpuToGpu, // Host visible coherent
			GpuToCpu
		};

		GpuAllocatedBuffer() = default;

		GpuAllocatedBuffer(uint32_t size, EBufferUsage usage, EMemoryUsage memoryUsage, GraphicsAllocator allocator);

		void Dispose(GraphicsAllocator allocator);

		[[nodiscard]]
		uint32_t GetSize() const noexcept;

		GpuAllocation GetAllocation();

		GpuBuffer GetBuffer();

		GpuAllocationInfo *GetAllocationInfo();

	private:
		GpuBuffer m_buffer = nullptr;
		GpuAllocation m_allocation = nullptr;
		std::shared_ptr<GpuAllocationInfo> m_allocInfo;
		uint32_t m_size = 0;
		uint32_t m_capacity = 0;
	};

} // namespace Hush

// NOLINTNEXTLINE
inline Hush::GpuAllocatedBuffer::EBufferUsage operator|(Hush::GpuAllocatedBuffer::EBufferUsage a, Hush::GpuAllocatedBuffer::EBufferUsage b)
{
    return static_cast<Hush::GpuAllocatedBuffer::EBufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}

// NOLINTNEXTLINE
inline Hush::GpuAllocatedBuffer::EBufferUsage operator&(Hush::GpuAllocatedBuffer::EBufferUsage a, Hush::GpuAllocatedBuffer::EBufferUsage b)
{
    return static_cast<Hush::GpuAllocatedBuffer::EBufferUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}

// NOLINTNEXTLINE
inline Hush::GpuAllocatedBuffer::EBufferUsage& operator|=(Hush::GpuAllocatedBuffer::EBufferUsage& a, Hush::GpuAllocatedBuffer::EBufferUsage b)
{
    a = a | b;
    return a;
}

