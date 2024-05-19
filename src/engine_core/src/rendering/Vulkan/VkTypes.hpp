/*! \file VkTypes.hpp
    \author Leonidas Gonzalez
    \date 2024-05-19
    \brief Type declaration for the Vulkan renderer
*/

#pragma once
#include <deque>
#include <functional>
#include <vulkan/vulkan.h>
#include "rendering/Vulkan/vk_mem_alloc.hpp"
namespace Hush
{
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

    struct VulkanDeletionQueue
    {
        std::deque<std::function<void()>> deletors;

        void PushFunction(std::function<void()> &&function)
        {
            deletors.push_back(function);
        }

        void Flush()
        {
            // reverse iterate the deletion queue to execute all the functions
            for (auto it = deletors.rbegin(); it != deletors.rend(); it++)
            {
                (*it)(); // call functors
            }

            deletors.clear();
        }
    };
}
