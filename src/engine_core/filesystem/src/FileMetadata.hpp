#pragma once

#include "serialization/Serialization.hpp"

namespace Hush
{

	// TODO: Let's do some unions and stuff

	/// @brief Describes the contents of a metadata file that links the resources
	struct FileMetadata
	{
		static inline constexpr uint16_t VERSION = 1;
		uint16_t metadataVersion;
		uint32_t id;

		template <class T>
		Serialization::ESerializationError Serialize(T &serializer) const
		{
			auto error = serializer.Serialize("metadataVersion", this->metadataVersion);
			if (error != Hush::Serialization::ESerializationError::None)
			{
				return error;
			}
			return serializer.Serialize("id", this->id);
		}
	};
} // namespace Hush
