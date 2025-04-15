#pragma once

#include "Shared/Types/ImageExtent3D.hpp"
#include <cstdint>
#if defined(HUSH_VULKAN_IMPL)
// Stuff from vk_mem_alloc to avoid cyclical references
struct VmaAllocation_T;
struct VkImage_T;
struct VkImageView_T;

using VmaAllocation = VmaAllocation_T *;
using VkImageView = VkImageView_T*;
using VkImage = VkImage_T*;

#endif

namespace Hush {
	
	struct GpuAllocatedImage
	{
		#if defined(HUSH_VULKAN_IMPL)
		VkImage image = nullptr;
		VkImageView imageView = nullptr;
		VmaAllocation allocation = nullptr;
		uint32_t imageFormat{};
		#endif
		ImageExtent3D imageExtent;
	};
}
