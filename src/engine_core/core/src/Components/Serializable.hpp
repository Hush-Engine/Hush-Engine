/// Describes a component as serializable, this must be added to the component's archetype itself,
//  not the entity that holds the component instance
#pragma once

#include "Assertions.hpp"
#include "Entity.hpp"
#include "serialization/Deserialization.hpp"
#include "serialization/Formats/JsonSerializer.hpp"
#include <array>
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
			MissingInternalResource,
			BufferOutOfMemory
		};

		EError (*serialize)(const uint8_t *self, Serialization::JsonSerializer &serializer, void *ctx) = nullptr;
		EError (*deserialize)(uint8_t *instance, Serialization::JsonDeserializer &deserializer, void *ctx) = nullptr;
		void (*postDeserialize)(uint8_t *instance, Entity::EntityId containerEntity, Entity::EntityId componentType,
								void *ctx) = nullptr;
		void *ctx = nullptr;
		static constexpr size_t LOCAL_BUFF_SIZE = 16;
		std::array<uint8_t, LOCAL_BUFF_SIZE> localBuffer;

		/// @brief Component type this serializable references
		Entity::EntityId type = Entity::INVALID_ENTITY_ID;

		bool usesLocalBuffer = false;

		void SetContextInline(const uint8_t *data, size_t size)
		{
			HUSH_ASSERT(size <= LOCAL_BUFF_SIZE,
						"Inline context size({}) is bigger than the component's local buffer size ({})", size,
						LOCAL_BUFF_SIZE);
			std::memcpy(this->localBuffer.data(), data, size);
			this->usesLocalBuffer = true;
		}

		void SetContextRef(void *ctx)
		{
			this->usesLocalBuffer = false;
			this->ctx = ctx;
		}

		/// @brief If your component (T) is registered with Hush's reflection system this function is more than enough
		template <class T>
		static EError DefaultSerialize(const uint8_t *self, Serialization::JsonSerializer &serializer,
									   void *ctx = nullptr)
		{
			(void)ctx;
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

		/// @brief If your component (T) is registered with Hush's reflection system this function is more than enough
		template <class T>
		static EError DefaultDeserialize(uint8_t *self, Serialization::JsonDeserializer &serializer,
										 void *ctx = nullptr)
		{
			(void)ctx;
			if (self == nullptr)
			{
				return Serializable::EError::BadInstance;
			}

			auto *comp = reinterpret_cast<T *>(self);
			Serialization::EDeserializationError res = serializer.Deserialize<T>(comp);

			if (res != Serialization::EDeserializationError::None)
			{
				return Serializable::EError::ParseError;
			}

			return Serializable::EError::None;
		}
	};
} // namespace Hush
