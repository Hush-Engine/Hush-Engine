#pragma once

#include <cstdint>

namespace Hush {
	// Flags
	enum class EAlphaBlendMode : uint32_t {
		None = 0,
		OneMinusSrcAlpha,
		OneMinusDestAlpha,
		ConstAlpha,
		DestAlpha,
		SrcAlpha
	};

	enum class ECullMode : uint32_t {
		None = 0,
		Front,
		Back,
		FrontAndBack = Front | Back
	};
}
