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

} // namespace Hush
