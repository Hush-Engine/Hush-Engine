#pragma once

/// @brief Generates bitwise flag operators for any enum type
#define HUSH_GENERATE_FLAGS(BaseEnumType, IntegerType)                                                                 \
	inline BaseEnumType operator|(BaseEnumType a, BaseEnumType b)                                                      \
	{                                                                                                                  \
		return static_cast<BaseEnumType>(static_cast<IntegerType>(a) | static_cast<IntegerType>(b));                   \
	}                                                                                                                  \
                                                                                                                       \
	inline BaseEnumType operator&(BaseEnumType a, BaseEnumType b)                                                      \
	{                                                                                                                  \
		return static_cast<BaseEnumType>(static_cast<IntegerType>(a) & static_cast<IntegerType>(b));                   \
	}                                                                                                                  \
                                                                                                                       \
	inline BaseEnumType &operator|=(BaseEnumType &a, BaseEnumType b)                                                   \
	{                                                                                                                  \
		a = a | b;                                                                                                     \
		return a;                                                                                                      \
	}

///@brief Definitions and all that just to make everything type safe
namespace Hush::Bitwise
{
	template <class T, class U>
	concept BitComparable = requires(T a, U b) { a & b; };

	template <class T, class U>
		requires BitComparable<T, U>
	constexpr inline bool HasFlag(T base, U flag)
	{
		return (base & flag) == flag;
	}

	template <class T, class U>
		requires BitComparable<T, U>
	constexpr inline bool HasCompositeFlag(T base, U compositeFlag)
	{
		return (base & compositeFlag) != 0;
	}
} // namespace Hush::Bitwise
