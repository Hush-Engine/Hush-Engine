#pragma once

// Comp metadata components go here

#include "crypto/Hashing.hpp"
#include "reflection/TypeId.hpp"
#include <cstdint>
#include <string_view>
#include "HushBindings.hpp"

namespace Hush {
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
