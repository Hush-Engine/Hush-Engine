/*! \file VkTypes.hpp
    \author Leonidas Gonzalez
    \date 2024-05-19
    \brief Type declaration for the Vulkan renderer
*/

#pragma once
#include <vulkan/vulkan.h>

// Stuff from vk_mem_alloc to avoid cyclical references
struct VmaAllocationInfo;
struct VmaAllocation_T;
using VmaAllocation = VmaAllocation_T*;

struct AllocatedImage
{
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};

struct AllocatedBuffer
{
    VkBuffer buffer;
    VmaAllocation allocation;
    VmaAllocationInfo info;
};
