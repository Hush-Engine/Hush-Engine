#pragma once
#include "Shared/MaterialPass.hpp"

#if defined (HUSH_VULKAN_IMPL)
namespace Hush {
	struct VkMaterialPipeline;
}
struct VkDescriptorSet_T;
using VkDescriptorSet = VkDescriptorSet_T*;
#endif

namespace Hush
{	
	struct GraphicsApiMaterialInstance
	{
		#if defined (HUSH_VULKAN_IMPL)
		VkMaterialPipeline *pipeline;
		VkDescriptorSet materialSet;
		#endif
		EMaterialPass passType;
	};
} // namespace Hush
