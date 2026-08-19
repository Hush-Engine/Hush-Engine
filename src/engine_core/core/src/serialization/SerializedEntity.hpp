#pragma once

#include "Entity.hpp"
#include "SerializedComponent.hpp"
#include <reflection/Type.hpp>
#include <serialization/Serialization.hpp>
#include <serialization/Deserialization.hpp>
#include <type_traits>

#if __has_include("SerializedEntity.hushgen.hpp") && !defined(HUSH_HEADER_PARSING)
#include "SerializedEntity.hushgen.hpp"
#endif
namespace Hush
{
	// PERF: Allocates quite a bit of memory
	struct [[hush::reflect]] SerializedEntity
	{
		HUSH_GENERATED_BODY
	public:
		[[hush::property]]
		Entity::EntityId id;
		[[hush::property]]
		std::string key;
		[[hush::property]]
		std::vector<SerializedComponent> components;
	};
} // namespace Hush
