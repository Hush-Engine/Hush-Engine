/*! \file TypeId.hpp
	\author Alan Ramirez
	\date 2025-04-20
	\brief TypeID implementation
*/

#pragma once

#include <cstdint>
#include <functional>

namespace Hush::Reflection
{
	struct TypeId
	{
		std::uint64_t id{};

		[[nodiscard]]
		bool operator==(const TypeId &other) const
		{
			return id == other.id;
		}

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
	std::size_t operator()(const Hush::Reflection::TypeId &typeId) const noexcept
	{
		return std::hash<std::uint64_t>{}(typeId.id);
	}
};