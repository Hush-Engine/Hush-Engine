/*! \file TypeTraits.hpp
	\author Alan Ramirez
	\date 2025-04-15
	\brief Hush Engine Type Traits
*/

#pragma once

#include <type_traits>
#include <cstdint>
#include <string_view>

#include "TypeId.hpp"

#include <crypto/Hashing.hpp>

namespace Hush
{
	class ReflectionDB;

	template <typename T>
	concept ReflectedType = requires(T t) {
		{ T::TypeId() } -> std::convertible_to<std::uint64_t>;
		{ T::TypeName() } -> std::same_as<std::string_view>;
	};

	namespace Reflection
	{
		template <ReflectedType T>
		/**
		 * @brief Retrieves the unique type identifier for a reflected type.
		 *
		 * Returns a TypeId constructed from the static TypeId() member of the type T, which must satisfy the ReflectedType concept.
		 *
		 * @return TypeId representing the unique identifier of the type T.
		 */
		constexpr TypeId GetTypeId()
		{
			return TypeId{T::TypeId()};
		}

		template <typename T>
		/**
		 * @brief Returns a default-constructed TypeId for types that do not satisfy the ReflectedType concept.
		 *
		 * This function provides a fallback for types without reflection metadata, resulting in an empty or invalid TypeId.
		 *
		 * @return TypeId Default-constructed (empty) type identifier.
		 */
		constexpr TypeId GetTypeId()
		{
			return {};
		}

		template <>
		/**
		 * @brief Returns the TypeId for the built-in type int32_t.
		 *
		 * Computes the TypeId using the FNV-1a 64-bit hash of the string "int32".
		 * @return TypeId corresponding to int32_t.
		 */
		constexpr TypeId GetTypeId<int32_t>()
		{
			return TypeId{Hush::Hashing::Fnv1a64("int32")};
		}

		template <>
		/**
		 * @brief Returns the unique TypeId for the void type.
		 *
		 * Computes the TypeId for `void` using the FNV-1a 64-bit hash of the string "void".
		 *
		 * @return TypeId representing the `void` type.
		 */
		inline constexpr TypeId GetTypeId<void>()
		{
			return TypeId{Hush::Hashing::Fnv1a64("void")};
		}
	} // namespace Reflection

} // namespace Hush