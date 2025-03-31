#pragma once

#include <cstdint>
#include <string_view>

namespace Hush::Hashing
{

	constexpr inline uint32_t Fnv1a(const char *data, const uint32_t length)
	{
		// NOLINTNEXTLINE
		uint32_t hash = 0x811c9dc5;
		const uint32_t prime = 0x1000193;

		for (int i = 0; i < length; ++i)
		{
			uint8_t value = data[i];
			hash = hash ^ value;
			hash *= prime;
		}

		return hash;
	}

	constexpr inline uint32_t Fnv1a(const std::string_view &data)
	{
		return Fnv1a(data.data(), data.size());
	}
} // namespace Hush::Hashing
