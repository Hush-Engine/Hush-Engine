/*! \file TypeTraits.hpp
	\author Alan Ramirez
	\date 2025-04-15
	\brief Hush Engine Type Traits
*/

#pragma once

#include <concepts>
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
		{ T::TypeId() } -> std::same_as<std::uint64_t>;
		{ T::TypeName() } -> std::same_as<std::string_view>;
		std::is_same_v<std::remove_cvref_t<T>, T>;
	};

	template <typename T>
	concept HasCustomTypeName = requires {
		{ T::TypeId() } -> std::same_as<std::string_view>;
	};

	namespace Reflection
	{
		// TODO: GetTypeId<T> falls back to an empty TypeId{} for any type that is neither a
		// reflected type nor one of the explicitly specialized primitives below. This includes
		// std::string and std::vector<T>, which means all of them currently share the same "no
		// type" identifier, breaking type-safety in the ReflectionDB. BACKLOG: implement proper
		// type ids for containers (e.g. via a partial-specializable provider struct).
		template <typename T>
		constexpr TypeId GetTypeId()
		{
			return {};
		}

		template <ReflectedType T>
		constexpr TypeId GetTypeId()
		{
			return TypeId{T::TypeId()};
		}

		template <>
		constexpr TypeId GetTypeId<uint8_t>()
		{
			return TypeId{Hush::Hashing::Fnv1a64("uint8")};
		}

		template <>
		constexpr TypeId GetTypeId<uint16_t>()
		{
			return TypeId{Hush::Hashing::Fnv1a64("uint16")};
		}

		template <>
		constexpr TypeId GetTypeId<uint32_t>()
		{
			return TypeId{Hush::Hashing::Fnv1a64("uint32")};
		}

		template <>
		constexpr TypeId GetTypeId<uint64_t>()
		{
			return TypeId{Hush::Hashing::Fnv1a64("uint64")};
		}

		template <>
		constexpr TypeId GetTypeId<int8_t>()
		{
			return TypeId{Hush::Hashing::Fnv1a64("int8")};
		}

		template <>
		constexpr TypeId GetTypeId<int16_t>()
		{
			return TypeId{Hush::Hashing::Fnv1a64("int16")};
		}

		template <>
		constexpr TypeId GetTypeId<int32_t>()
		{
			return TypeId{Hush::Hashing::Fnv1a64("int32")};
		}

		template <>
		constexpr TypeId GetTypeId<int64_t>()
		{
			return TypeId{Hush::Hashing::Fnv1a64("int64")};
		}

		template <>
		constexpr TypeId GetTypeId<bool>()
		{
			return TypeId{Hush::Hashing::Fnv1a64("bool")};
		}

		template <>
		constexpr TypeId GetTypeId<float>()
		{
			return TypeId{Hush::Hashing::Fnv1a64("float")};
		}

		template <>
		constexpr TypeId GetTypeId<double>()
		{
			return TypeId{Hush::Hashing::Fnv1a64("double")};
		}

		template <>
		constexpr TypeId GetTypeId<void>()
		{
			// void type is a special type, it has the value "0", so it means "no type"
			return TypeId{};
		}

		template <>
		constexpr TypeId GetTypeId<std::string_view>()
		{
			return TypeId{Hush::Hashing::Fnv1a64("std::string_view")};
		}

	} // namespace Reflection

} // namespace Hush
