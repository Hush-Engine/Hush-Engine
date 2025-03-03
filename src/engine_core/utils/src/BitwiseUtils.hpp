#pragma once

///@brief Definitions and all that just to make everything type safe
namespace Hush::Bitwise {
	template<class T, class U>
	concept BitComparable = requires(T a, U b) {
		a & b;
	};

	template<class T, class U>
	requires BitComparable<T, U>
	constexpr inline bool HasFlag(T base, U flag) {
		return (base & flag) == flag;
	}

	template<class T, class U>
	requires BitComparable<T, U>
	constexpr inline bool HasCompositeFlag(T base, U compositeFlag) {
		return (base & compositeFlag) != 0;
	}
};