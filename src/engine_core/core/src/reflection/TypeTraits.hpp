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
		constexpr TypeId GetTypeId()
		{
			return TypeId{T::TypeId()};
		}

		template <typename T>
		constexpr TypeId GetTypeId()
		{
			return {};
		}

		template <>
		constexpr TypeId GetTypeId<int32_t>()
		{
			return TypeId{Hush::Hashing::Fnv1a64("int32")};
		}

		template <>
		inline constexpr TypeId GetTypeId<void>()
		{
			return TypeId{Hush::Hashing::Fnv1a64("void")};
		}
	} // namespace Reflection

} // namespace Hush