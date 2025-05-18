#pragma once
#include "Shared/Types/MaterialInstance.hpp"
#include <glm/mat4x4.hpp>
#include <vulkan/vulkan_core.h>

namespace Hush
{
	struct VkRenderObject
	{
		uint32_t indexCount;
		uint32_t firstIndex;
		VkBuffer indexBuffer;

		GraphicsApiMaterialInstance *material;
		glm::mat4 transform;
		VkDeviceAddress vertexBufferAddress;
	};
} // namespace Hush
