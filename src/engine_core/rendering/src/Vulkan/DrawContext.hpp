#pragma once
#include "VkRenderObject.hpp"
#include <vector>

namespace Hush
{
	struct DrawContext
	{
		std::vector<VkRenderObject> opaqueSurfaces;
		std::vector<VkRenderObject> transparentSurfaces;
	};
} // namespace Hush
