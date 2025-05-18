#pragma once

#include <cstdint>
#include <string_view>

namespace Hush::Hashing
{

	/**
	 * @brief Computes a 32-bit FNV-1a hash for a byte array.
	 *
	 * @param data Pointer to the input data.
	 * @param length Number of bytes to hash.
	 * @return 32-bit FNV-1a hash of the input data.
	 */
	constexpr uint32_t Fnv1a(const char *data, const uint32_t length)
	{
		// NOLINTNEXTLINE
		uint32_t hash = 0x811c9dc5;
		const uint32_t prime = 0x1000193;

		for (uint32_t i = 0; i < length; ++i)
		{
			uint8_t value = data[i];
			hash = hash ^ value;
			hash *= prime;
		}

		return hash;
	}

	/**
	 * @brief Computes a 64-bit FNV-1a hash for a byte array.
	 *
	 * @param data Pointer to the input byte array.
	 * @param length Number of bytes to hash.
	 * @return 64-bit FNV-1a hash of the input data.
	 */
	constexpr std::uint64_t Fnv1a64(const char *data, const uint32_t length)
	{
		// NOLINTNEXTLINE
		std::uint64_t hash = 0xcbf29ce484222325;
		const std::uint64_t prime = 0x100000001b3;

		for (uint32_t i = 0; i < length; ++i)
		{
			uint8_t value = data[i];
			hash = hash ^ value;
			hash *= prime;
		}

		return hash;
	}

	/**
	 * @brief Computes the 32-bit FNV-1a hash of a string view.
	 *
	 * @param data The input data as a string view.
	 * @return uint32_t The 32-bit FNV-1a hash value.
	 */
	constexpr uint32_t Fnv1a(const std::string_view &data)
	{
		return Fnv1a(data.data(), static_cast<uint32_t>(data.size()));
	}

	/**
	 * @brief Computes the 64-bit FNV-1a hash of a string view.
	 *
	 * @param data The input data to hash.
	 * @return std::uint64_t The 64-bit FNV-1a hash value.
	 */
	constexpr std::uint64_t Fnv1a64(const std::string_view &data)
	{
		return Fnv1a64(data.data(), static_cast<uint32_t>(data.size()));
	}
} // namespace Hush::Hashing
