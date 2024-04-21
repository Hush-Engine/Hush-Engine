/*! \file VkUtilsFactory.hpp
    \author Leonidas Gonzalez
    \date 2024-04-06
    \brief Provides initializers for various Vulkan structures
*/

#pragma once
#include <vulkan/vulkan.h>

class VkUtilsFactory
{
  public:
    static VkFenceCreateInfo CreateFenceInfo(VkFenceCreateFlagBits flags)
    {
        VkFenceCreateInfo result = {};
        result.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        result.pNext = nullptr;
        result.flags = flags;

        return result;
    }

    static VkSemaphoreCreateInfo CreateSemaphoreInfo(VkSemaphoreCreateFlags flags = 0u)
    {
        VkSemaphoreCreateInfo result = {};
        result.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;
        result.pNext = nullptr;
        result.flags = flags;

        return result;
    }

    static VkRenderingAttachmentInfoKHR CreateRenderingAttachmentInfo()
    {
        VkRenderingAttachmentInfoKHR result{};
        result.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        result.pNext = nullptr;
        return result;
    }

    static VkCommandPoolCreateInfo CreateCommandPoolInfo(uint32_t queueFamilyIndex, VkCommandPoolCreateFlagBits flags)
    {
        VkCommandPoolCreateInfo commandPoolInfo = {};
        commandPoolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        commandPoolInfo.pNext = nullptr;
        commandPoolInfo.flags = flags;
        commandPoolInfo.queueFamilyIndex = queueFamilyIndex;
    }

    static VkCommandBufferAllocateInfo CreateCommandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1u)
    {
        VkCommandBufferAllocateInfo cmdAllocInfo = {};
        cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocInfo.pNext = nullptr;
        cmdAllocInfo.commandPool = pool;
        cmdAllocInfo.commandBufferCount = count;
        cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
    }

    static VkCommandBufferBeginInfo CreateCommandBufferBeginInfo(VkCommandBufferUsageFlags flags = 0u)
    {
        VkCommandBufferBeginInfo info = {};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        info.pNext = nullptr;

        info.pInheritanceInfo = nullptr;
        info.flags = flags;
        return info;
    }

    static VkSemaphoreSubmitInfo CreateSemaphoreSubmitInfo(VkPipelineStageFlags2 stageMask, VkSemaphore semaphore)
    {
        VkSemaphoreSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_SUBMIT_INFO;
        submitInfo.pNext = nullptr;
        submitInfo.semaphore = semaphore;
        submitInfo.stageMask = stageMask;
        submitInfo.deviceIndex = 0;
        submitInfo.value = 1;

        return submitInfo;
    }

    static VkCommandBufferSubmitInfo CreateCommandBufferSubmitInfo(VkCommandBuffer cmd)
    {
        VkCommandBufferSubmitInfo info{};
        info.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_SUBMIT_INFO;
        info.pNext = nullptr;
        info.commandBuffer = cmd;
        info.deviceMask = 0;

        return info;
    }

    static VkSubmitInfo2 SubmitInfo(VkCommandBufferSubmitInfo *cmd, VkSemaphoreSubmitInfo *signalSemaphoreInfo,
                                    VkSemaphoreSubmitInfo *waitSemaphoreInfo)
    {
        VkSubmitInfo2 info = {};
        info.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO_2;
        info.pNext = nullptr;

        info.waitSemaphoreInfoCount = waitSemaphoreInfo == nullptr ? 0 : 1;
        info.pWaitSemaphoreInfos = waitSemaphoreInfo;

        info.signalSemaphoreInfoCount = signalSemaphoreInfo == nullptr ? 0 : 1;
        info.pSignalSemaphoreInfos = signalSemaphoreInfo;

        info.commandBufferInfoCount = 1;
        info.pCommandBufferInfos = cmd;

        return info;
    }

    static VkRenderingAttachmentInfoKHR CreateColorAttachmentInfo(VkImageView currentSwapchainImageView,
                                                                  VkClearValue clearValue)
    {
        VkRenderingAttachmentInfoKHR colorAttachmentInfo = {};

        colorAttachmentInfo.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO_KHR;
        colorAttachmentInfo.imageView = currentSwapchainImageView;
        colorAttachmentInfo.imageLayout = VK_IMAGE_LAYOUT_ATTACHMENT_OPTIMAL_KHR;
        colorAttachmentInfo.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachmentInfo.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentInfo.clearValue = clearValue;

        return colorAttachmentInfo;
    }
};
