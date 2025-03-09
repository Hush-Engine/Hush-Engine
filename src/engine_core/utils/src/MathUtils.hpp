#pragma once
#include <cmath>

namespace Hush::MathUtils
{

	inline float Clamp(float value, float min, float max)
	{
		const float t = value < min ? min : value;
		return t > max ? max : t;
	}

	inline int32_t CircleBack(int32_t value, int32_t min, int32_t maxInclusive) {
		if (value < min) {
			return maxInclusive;
		}
		if (value > maxInclusive) {
			return min;
		}
		return value;
	}
	
	inline float Lerp(float from, float to, float t)
	{
		const float tClamped = Clamp(t, 0.0f, 1.0f);
		return from + (to - from) * tClamped;
	}

	inline float Pow(float x, float n)
	{
		return pow(x, n);
	}
} // namespace Hush::MathUtils
