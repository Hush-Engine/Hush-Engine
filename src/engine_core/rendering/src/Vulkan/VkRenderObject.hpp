#pragma once
#include "VkMaterialInstance.hpp"
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
