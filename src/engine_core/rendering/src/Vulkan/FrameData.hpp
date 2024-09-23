/*! \file FrameData.hpp
    \author Kyn21kx
    \date 2024-05-20
    \brief Frame data to be passed to vulkan's dynamic rendering
*/

#pragma once
#include "VulkanDeletionQueue.hpp"
#include <vulkan/vulkan.h>
#include "VkDescriptors.hpp"

/// @brief Definition of the frame data structure to pass in Vulkan's dynamic rendering
/// from VKGuide (https://vkguide.dev/docs/new_chapter_1/vulkan_mainloop_code/)
struct FrameData
{
    /// @brief Used for making the render commands wait for the swapchain image
    VkSemaphore swapchainSemaphore;

    /// @brief Controls presenting the image to the OS once drawing is finished
    VkSemaphore renderSemaphore;

    /// @brief Lets us wait for the draw commands of a given frame to be finished.
    VkFence renderFence;

    VkCommandBuffer mainCommandBuffer;
    VkCommandPool commandPool;
    VulkanDeletionQueue deletionQueue;

    DescriptorAllocatorGrowable frameDescriptors;

};