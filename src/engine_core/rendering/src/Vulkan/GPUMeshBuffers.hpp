#pragma once
#include "Shared/GpuAllocatedBuffer.hpp"
#include <vulkan/vulkan_core.h>

namespace Hush
{

	// holds the resources needed for a mesh
	struct GPUMeshBuffers
	{
		GPUMeshBuffers() = default;

		GpuAllocatedBuffer indexBuffer;
		GpuAllocatedBuffer vertexBuffer;
		#if defined(HUSH_VULKAN_IMPL)
		uint64_t vertexBufferAddress = 0U;
		#endif
	};

} // namespace Hush
