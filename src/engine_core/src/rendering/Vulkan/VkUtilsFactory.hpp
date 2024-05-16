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
        return commandPoolInfo;
    }

    static VkCommandBufferAllocateInfo CreateCommandBufferAllocateInfo(VkCommandPool pool, uint32_t count = 1u)
    {
        VkCommandBufferAllocateInfo cmdAllocInfo = {};
        cmdAllocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        cmdAllocInfo.pNext = nullptr;
        cmdAllocInfo.commandPool = pool;
        cmdAllocInfo.commandBufferCount = count;
        cmdAllocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        return cmdAllocInfo;
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

    static VkRenderingAttachmentInfo CreateAttachmentInfoWithLayout(VkImageView view, VkClearValue *clear, VkImageLayout layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL)
    {    
     VkRenderingAttachmentInfo colorAttachment{};
        colorAttachment.sType = VK_STRUCTURE_TYPE_RENDERING_ATTACHMENT_INFO;
        colorAttachment.pNext = nullptr;

        colorAttachment.imageView = view;
        colorAttachment.imageLayout = layout;
        colorAttachment.loadOp = clear != nullptr ? VK_ATTACHMENT_LOAD_OP_CLEAR : VK_ATTACHMENT_LOAD_OP_LOAD;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        if (clear != nullptr)
        {
            colorAttachment.clearValue = *clear;
        }

        return colorAttachment;
    }

    static VkRenderingInfo CreateRenderingInfo(VkExtent2D renderExtent, VkRenderingAttachmentInfo *colorAttachment,
                                               VkRenderingAttachmentInfo *depthAttachment)
    {
    
     VkRenderingInfo renderInfo{};
        renderInfo.sType = VK_STRUCTURE_TYPE_RENDERING_INFO;
        renderInfo.pNext = nullptr;

        renderInfo.renderArea = VkRect2D{VkOffset2D{0, 0}, renderExtent};
        renderInfo.layerCount = 1;
        renderInfo.colorAttachmentCount = 1;
        renderInfo.pColorAttachments = colorAttachment;
        renderInfo.pDepthAttachment = depthAttachment;
        renderInfo.pStencilAttachment = nullptr;

        return renderInfo;
    }

    static VkDebugUtilsMessengerCreateInfoEXT CreateDebugMessengerInfo(PFN_vkDebugUtilsMessengerCallbackEXT callback, void* pUserData = nullptr)
    {
        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo = {};
        debugCreateInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        debugCreateInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT |
                                          VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT;
        debugCreateInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                      VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        debugCreateInfo.pfnUserCallback = callback; // This is a function you need to define
        debugCreateInfo.pUserData = pUserData;             // Optional data pointer
        return debugCreateInfo;
    }

};
