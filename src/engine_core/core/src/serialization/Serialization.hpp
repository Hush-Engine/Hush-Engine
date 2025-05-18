/*! \file Serialization.hpp
	\author Alan Ramirez
	\date 2025-04-18
	\brief Serialization/deserialization types
*/

#pragma once

#include <Result.hpp>
#include <cstdint>
#include <concepts>
#include <optional>
#include <utility>

namespace Hush::Serialization
{
	// Serializers support serializing data in a specific format.
	// To see an implementation of a serializer, see @ref JsonSerializer. It includes all the required member functions
	// to serialize data.

	///
	/// SerializationError enum class
	enum class ESerializationError
	{
		None = 0,
		InvalidType = 1,
		InvalidData = 2,
		InvalidFormat = 3,
	};

	template <typename A, typename T>
	concept IsSerializer = requires(A a, T t) {
		{ t.Serialize(a) } -> std::same_as<ESerializationError>;
	};

	template <typename T, typename U>
	concept IsSerializable = requires(const T &t, U &u) {
		{ t.Serialize(u) } -> std::same_as<ESerializationError>;
	};

	template <typename It, typename K, typename V>
	concept IsMapIterator = requires(It it) {
		typename It::value_type;
		requires std::same_as<typename It::value_type, std::pair<K, V>>;
		{ *it } -> std::convertible_to<typename It::value_type>;
	};
} // namespace Hush::Serialization