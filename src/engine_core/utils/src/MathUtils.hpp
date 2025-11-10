#pragma once
#include <cmath>
#include <numbers>

namespace Hush::MathUtils
{
	constexpr float PI = std::numbers::pi_v<float>;
	constexpr float TAU = PI * 2.0f;
	constexpr float RAD_TO_DEG = 180.0f / PI;
	constexpr float DEG_TO_RAD = PI / 180.0f;

	inline float Clamp(float value, float min, float max)
	{
		const float t = value < min ? min : value;
		return t > max ? max : t;
	}

	inline int32_t CircleBack(int32_t value, int32_t min, int32_t maxInclusive)
	{
		if (value < min)
		{
			return maxInclusive;
		}
		if (value > maxInclusive)
		{
			return min;
		}
		return value;
	}

	inline float Lerp(float from, float to, float t)
	{
		const float tClamped = Clamp(t, 0.0f, 1.0f);
		return from + ((to - from) * tClamped);
	}

	inline float Pow(float x, float n)
	{
		return pow(x, n);
	}
} // namespace Hush::MathUtils
