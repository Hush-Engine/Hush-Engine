#pragma once

#include "Types/Color.hpp"
#include "Vector4Math.hpp"

namespace Hush
{
	struct DirectionalLight
	{
		float intensity = 1.0F;
		Color color = Vector4Math::ONE;
	};

	void Serialize(DirectionalLight *component);
} // namespace Hush
