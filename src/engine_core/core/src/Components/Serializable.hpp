/// Describes a component as serializable, this must be added to the component's archetype itself,
//  not the entity that holds the component instance
#pragma once

#include "Entity.hpp"
#include <cstdint>
#include <span>
#include <string_view>
namespace Hush
{
	struct Serializable
	{
		enum class EError
		{
			None = 0,
			ParseError,
			BadInstance,
			BufferOutOfMemory
		};

		EError(*serialize)(const uint8_t* self, std::span<char> buffer);
		EError(*deserialize)(uint8_t* instance, const std::string_view& data);

		/// @brief Component type this serializable references
		Entity::EntityId type;
		
	};
} // namespace Hush
