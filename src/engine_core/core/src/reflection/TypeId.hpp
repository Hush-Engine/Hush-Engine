/*! \file TypeId.hpp
	\author Alan Ramirez
	\date 2025-04-20
	\brief TypeID implementation
*/

#pragma once

namespace Hush::Reflection
{
	struct TypeId
	{
		std::uint64_t id{};

		/**
		 * @brief Checks if two TypeId instances are equal.
		 *
		 * Compares the underlying id values of both TypeId objects for equality.
		 *
		 * @param other The TypeId instance to compare with.
		 * @return true if both TypeId objects have the same id; otherwise, false.
		 */
		[[nodiscard]]
		bool operator==(const TypeId &other) const
		{
			return id == other.id;
		}

		/**
		 * @brief Checks if two TypeId instances represent different type identifiers.
		 *
		 * @param other The TypeId to compare with.
		 * @return true if the ids are not equal, false otherwise.
		 */
		[[nodiscard]]
		bool operator!=(const TypeId &other) const
		{
			return id != other.id;
		}
	};
}

template<>
struct std::hash<Hush::Reflection::TypeId>
{
	/**
	 * @brief Computes a hash value for a TypeId.
	 *
	 * Applies std::hash to the underlying 64-bit identifier of the TypeId.
	 *
	 * @param typeId The TypeId instance to hash.
	 * @return std::size_t The hash value of the TypeId.
	 */
	std::size_t operator()(const Hush::Reflection::TypeId &typeId) const noexcept
	{
		return std::hash<std::uint64_t>{}(typeId.id);
	}
};