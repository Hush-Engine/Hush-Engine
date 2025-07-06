#pragma once

#include "BitwiseUtils.hpp"
#include <cstddef>
#include <cstdint>

namespace Hush
{

	class GpuAllocatedBuffer final
	{
	public:
		struct OpaqueAllocationData;

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

		GpuAllocatedBuffer(size_t size, EBufferUsage usage, EMemoryUsage memoryUsage, void *allocator);

		void Dispose(void *allocator);

		[[nodiscard]]
		size_t GetSize() const noexcept;

		const OpaqueAllocationData *GetAllocationData();

		void *GetBuffer();

		void *GetMappedData();

	private:
		size_t m_offset{};
		void *m_mappedData{};
		void *m_userData{};
		void *m_buffer{};
		void *m_allocation{}; // Used for VmaAllocation_T* on Vulkan
		size_t m_size = 0;
		size_t m_capacity = 0;
	};

} // namespace Hush

// NOLINTNEXTLINE
HUSH_GENERATE_FLAGS(Hush::GpuAllocatedBuffer::EBufferUsage, uint32_t);
