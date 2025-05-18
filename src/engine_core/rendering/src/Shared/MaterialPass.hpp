#pragma once
#include <cstdint>
namespace Hush
{
	enum class EMaterialPass : uint8_t
	{
		MainColor,
		Transparent,
		Mask,
		Other
	};
} // namespace Hush
