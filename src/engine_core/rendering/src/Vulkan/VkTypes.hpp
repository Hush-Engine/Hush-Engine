/*! \file VkTypes.hpp
    \author Leonidas Gonzalez
    \date 2024-05-19
    \brief Type declaration for the Vulkan renderer
*/

#pragma once
#include "Assertions.hpp"
#include <vulkan/vulkan.h>
#include <magic_enum/magic_enum.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>


// Stuff from vk_mem_alloc to avoid cyclical references
struct VmaAllocation_T;
using VmaAllocation = VmaAllocation_T *;

struct Vertex {
	glm::vec3 position;
	float uv_x = 0.0f;
	glm::vec3 normal = { 1, 0, 0 };
	float uv_y = 0.0f;
	glm::vec4 color = glm::vec4{ 1.f };
};

// push constants for our mesh object draws
struct GPUDrawPushConstants {
	glm::mat4 worldMatrix;
	VkDeviceAddress vertexBuffer;
};

struct AllocatedImage
{
	VkImage image;
	VkImageView imageView;
	VmaAllocation allocation;
	VkExtent3D imageExtent;
	VkFormat imageFormat;
};

struct ComputePushConstants {
	glm::vec4 data1;
	glm::vec4 data2;
	glm::vec4 data3;
	glm::vec4 data4;
};

//MAX bytes that we're able to pass to the GPU per shader
static_assert(sizeof(ComputePushConstants) <= 128, "Compute shader data exceeds the size limit per shader (128 bytes)");

#define HUSH_VK_ASSERT(result, message)                                                                                \
    HUSH_ASSERT((result) == VkResult::VK_SUCCESS, "{} VK error code: {}", message, magic_enum::enum_name(result))
