#pragma once

#include <cstdint>
namespace Hush
{
	struct ImageExtent3D
	{
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t depth = 0;

		constexpr ImageExtent3D() = default;

		constexpr ImageExtent3D(uint32_t width, uint32_t height, uint32_t depth)
			: width(width),
			  height(height),
			  depth(depth)
		{
		}

		constexpr ImageExtent3D(uint32_t scalar)
			: width(scalar),
			  height(scalar),
			  depth(scalar)
		{
		}
	};
} // namespace Hush
