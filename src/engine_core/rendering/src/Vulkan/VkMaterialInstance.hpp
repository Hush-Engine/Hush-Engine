#pragma once
#include <vulkan/vulkan_core.h>
#include "Shared/MaterialPass.hpp"

namespace Hush
{

	struct VkMaterialPipeline
	{
		VkPipeline pipeline;
		VkPipelineLayout layout; // Check deletion queue?
	};

	struct VkMaterialInstance
	{
		VkMaterialPipeline *pipeline;
		VkDescriptorSet materialSet;
		EMaterialPass passType;
	};
} // namespace Hush
