/// Describes a component as serializable, this must be added to the component's archetype itself,
//  not the entity that holds the component instance
#pragma once

#include "Entity.hpp"
#include "serialization/Formats/JsonSerializer.hpp"
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

		EError (*serialize)(const uint8_t *self, Serialization::JsonSerializer &serializer);
		EError (*deserialize)(uint8_t *instance, const std::string_view &data);

		/// @brief Component type this serializable references
		Entity::EntityId type;

		/// @brief If your component (T) is registered with Hush's reflection system this function is more than enough
		template <class T>
		static EError DefaultSerialize(const uint8_t *self, Serialization::JsonSerializer &serializer)
		{
			if (self == nullptr)
			{
				return Serializable::EError::BadInstance;
			}

			const auto *comp = reinterpret_cast<const T *>(self);
			Serialization::ESerializationError err = serializer.Serialize(*comp, false);

			if (err != Serialization::ESerializationError::None)
			{
				return Serializable::EError::ParseError;
			}

			return Serializable::EError::None;
		}
	};
} // namespace Hush
