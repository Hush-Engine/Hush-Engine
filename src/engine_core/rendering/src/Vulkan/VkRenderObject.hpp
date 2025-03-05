#pragma once
#include "VkMaterialInstance.hpp"
#include <glm/mat4x4.hpp>

namespace Hush
{
	struct VkRenderObject
	{
		uint32_t indexCount;
		uint32_t firstIndex;
		VkBuffer indexBuffer;

		VkMaterialInstance *material;
		glm::mat4 transform;
		VkDeviceAddress vertexBufferAddress;
	};
} // namespace Hush
