/*! \file VkTypes.hpp
    \author Leonidas Gonzalez
    \date 2024-05-19
    \brief Type declaration for the Vulkan renderer
*/

#pragma once
#include "utils/Assertions.hpp"
#include <vulkan/vulkan.h>
#include <magic_enum.hpp>

// Stuff from vk_mem_alloc to avoid cyclical references
struct VmaAllocation_T;
using VmaAllocation = VmaAllocation_T *;

struct AllocatedImage
{
    VkImage image;
    VkImageView imageView;
    VmaAllocation allocation;
    VkExtent3D imageExtent;
    VkFormat imageFormat;
};

#ifndef HUSH_VULKAN_IMPL
#define HUSH_VULKAN_IMPL
// NOLINTNEXTLINE
#define HUSH_VK_ASSERT(result, message)                                                                                \
    HUSH_ASSERT((result) == VkResult::VK_SUCCESS, "{} VK error code: {}", message, magic_enum::enum_name(result))
#endif