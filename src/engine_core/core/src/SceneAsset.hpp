#pragma once
// This is in the core module even though it's a resource because
// the core engine should still know how to convert it to a Hush::Scene*

#include "Entity.hpp"
#include <cstdint>
#include <vector>

namespace Hush
{

	//TODO: This is our first implementation of scene assets, it is highly subject to change
	// Also, this class allocates like crazy, so, watch out for that
	class SceneAsset
	{
	public:
		struct SerializedComponent {
			std::string type;
			// JSON needs to be turned into component data at runtime, we shouldn't own a void* or any other templated type into it
			std::string jsonData;
		};


		// PERF: Allocates quite a bit of memory
		struct SerializedEntity {
			Entity::EntityId id;
			std::string key;
			std::vector<SerializedComponent> components;
		};

		struct SerializedSystem {
			std::string type;
			int32_t order;
		};

		// Some metadata
		uint8_t versionMajor;
		uint8_t versionMinor;

		/// @brief UUID that corresponds to this scene asset
		uint64_t sceneId;

		// Systems
		std::vector<SerializedSystem> systems;

		// entities
		std::vector<SerializedEntity> entities;

	};

} // namespace Hush
