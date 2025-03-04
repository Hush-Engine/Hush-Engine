/*! \file EnumFlags.hpp
	\author Alan Ramirez Herrera
	\date 2024-02-01
	\brief Enum flags helper
*/

#pragma once

#include <type_traits>

namespace Hush
{
	/// Trait to enable bit mask operators for an enum.
	/// To implement this trait, specialize the EnableBitMaskOperators struct for the enum.
	/// There's a helper macro to do this in the EnumFlags.hpp file. Use HUSH_ENABLE_BITMASK_OPERATORS(Enum)
	/// However, we recommend implementing it manually to avoid macros.
	template <typename Enum>
	struct EnableBitMaskOperators : std::false_type
	{
	};

	/// Helper variable template to check if an enum has bit mask operators.
	/// @tparam Enum Enum type to check if it has bit mask operators.
	template <typename Enum>
	constexpr bool HAS_BIT_MASK_OPERATORS = EnableBitMaskOperators<Enum>::value;

	/// Helper concepts to check if an enum has bit mask operators.
	/// @tparam Enum Enum type to check if it has bit mask operators.
	template <typename Enum>
	concept BitMaskEnum = std::is_enum_v<Enum> && HAS_BIT_MASK_OPERATORS<Enum>;
}; // namespace Hush

/// Bitwise OR operator for enums with bit mask operators.
/// @tparam Enum Enum type.
/// @param lhs Left hand side of the operator.
/// @param rhs Right hand side of the operator.
/// @return Result of the operation.
template <Hush::BitMaskEnum Enum>
constexpr Enum operator|(Enum lhs, Enum rhs)
{
	using Underlying = std::underlying_type_t<Enum>;
	return static_cast<Enum>(static_cast<Underlying>(lhs) | static_cast<Underlying>(rhs));
}

/// Bitwise AND operator for enums with bit mask operators.
/// @tparam Enum Enum type.
/// @param lhs Left hand side of the operator.
/// @param rhs Right hand side of the operator.
/// @return Result of the operation.
template <Hush::BitMaskEnum Enum>
constexpr Enum operator&(Enum lhs, Enum rhs)
{
	using Underlying = std::underlying_type_t<Enum>;
	return static_cast<Enum>(static_cast<Underlying>(lhs) & static_cast<Underlying>(rhs));
}

/// Bitwise XOR operator for enums with bit mask operators.
/// @tparam Enum Enum type.
/// @param lhs Left hand side of the operator.
/// @param rhs Right hand side of the operator.
/// @return Result of the operation.
template <Hush::BitMaskEnum Enum>
constexpr Enum operator^(Enum lhs, Enum rhs)
{
	using Underlying = std::underlying_type_t<Enum>;
	return static_cast<Enum>(static_cast<Underlying>(lhs) ^ static_cast<Underlying>(rhs));
}

/// Bitwise NOT operator for enums with bit mask operators.
/// @tparam Enum Enum type.
/// @param rhs Right hand side of the operator.
/// @return Result of the operation.
template <Hush::BitMaskEnum Enum>
constexpr Enum operator~(Enum rhs)
{
	using Underlying = std::underlying_type_t<Enum>;
	return static_cast<Enum>(~static_cast<Underlying>(rhs));
}

/// Bitwise OR assignment operator for enums with bit mask operators.
/// @tparam Enum Enum type.
/// @param lhs Left hand side of the operator.
/// @param rhs Right hand side of the operator.
/// @return lhs.
template <Hush::BitMaskEnum Enum>
constexpr Enum &operator|=(Enum &lhs, Enum rhs)
{
	using Underlying = std::underlying_type_t<Enum>;
	lhs = static_cast<Enum>(static_cast<Underlying>(lhs) | static_cast<Underlying>(rhs));
	return lhs;
}

/// Bitwise AND assignment operator for enums with bit mask operators.
/// @tparam Enum Enum type.
/// @param lhs Left hand side of the operator.
/// @param rhs Right hand side of the operator.
/// @return lhs.
template <Hush::BitMaskEnum Enum>
constexpr Enum &operator&=(Enum &lhs, Enum rhs)
{
	using Underlying = std::underlying_type_t<Enum>;
	lhs = static_cast<Enum>(static_cast<Underlying>(lhs) & static_cast<Underlying>(rhs));
	return lhs;
}

/// Bitwise XOR assignment operator for enums with bit mask operators.
/// @tparam Enum Enum type.
/// @param lhs Left hand side of the operator.
/// @param rhs Right hand side of the operator.
/// @return lhs.
template <Hush::BitMaskEnum Enum>
constexpr Enum &operator^=(Enum &lhs, Enum rhs)
{
	using Underlying = std::underlying_type_t<Enum>;
	lhs = static_cast<Enum>(static_cast<Underlying>(lhs) ^ static_cast<Underlying>(rhs));
	return lhs;
}

namespace Hush
{
	/// Check if an enum value has a flag.
	/// @tparam Enum Enum type.
	/// @param value Value to check.
	/// @param flag Flag to check.
	/// @return True if the value has the flag, false otherwise.
	template <BitMaskEnum Enum>
	constexpr bool HasFlag(Enum value, Enum flag)
	{
		return (value & flag) == flag;
	}

	/// Check if an enum value has any of the flags.
	/// @tparam Enum Enum type.
	/// @param value Value to check.
	/// @param flag Flag to check.
	/// @return True if the value has any of the flags, false otherwise.
	template <BitMaskEnum Enum>
	constexpr bool HasAnyFlag(Enum value, Enum flag)
	{
		return (value & flag) != Enum{};
	}

	/// Check if an enum value has all the flags.
	/// @tparam Enum Enum type.
	/// @param value Value to check.
	/// @param flags Flags to check.
	/// @return True if the value has all the flags, false otherwise.
	template <BitMaskEnum Enum>
	constexpr bool HasAllFlags(Enum value, Enum flags)
	{
		return (value & flags) == flags;
	}

	/// Helper macro to enable bit mask operators for an enum.
#define HUSH_ENABLE_BITMASK_OPERATORS(Enum)                                                                            \
	template <>                                                                                                        \
	struct EnableBitMaskOperators<Enum> : std::true_type                                                               \
	{                                                                                                                  \
	};
} // namespace Hush