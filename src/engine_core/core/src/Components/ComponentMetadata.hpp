#pragma once

// Comp metadata components go here

#include "crypto/Hashing.hpp"
#include "reflection/TypeId.hpp"
#include <cstdint>
#include <string_view>
#include "HushBindings.hpp"

namespace Hush {


	/// @brief Mirrors ECompPropertyType from the scripting library's compile-time serialization registry
	enum EComponentPropertyType : uint64_t
	{
		Unknown = 0,
		I8 = Hashing::Fnv1a64("int8_t"),
		U8 = Hashing::Fnv1a64("uint8_t"),
		I16 = Hashing::Fnv1a64("int16_t"),
		U16 = Hashing::Fnv1a64("uint16_t"),
		I32 = Hashing::Fnv1a64("int32_t"),
		U32 = Hashing::Fnv1a64("uint32_t"),
		I64 = Hashing::Fnv1a64("int64_t"),
		U64 = Hashing::Fnv1a64("uint64_t"),
		F32 = Hashing::Fnv1a64("float"),
		F64 = Hashing::Fnv1a64("double"),
		Bool = Hashing::Fnv1a64("bool"),
		String = Hashing::Fnv1a64("String"),
		Array = Hashing::Fnv1a64("Array"),
		Vector3 = Hashing::Fnv1a64("Vector3"),
		Vector2 = Hashing::Fnv1a64("Vector2")
	};
	
	struct [[hush::export]] InspectableComponent
	{
		[[hush::export]]
		Hush::Reflection::TypeId reflectedTypeId;

		static constexpr std ::string_view TypeName()
		{
			return "InspectableComponent";
		}

		static constexpr std::uint64_t TypeId() {
			return Hashing::Fnv1a64(TypeName());
		}

	}; // Raw tag for user-defined components
}
