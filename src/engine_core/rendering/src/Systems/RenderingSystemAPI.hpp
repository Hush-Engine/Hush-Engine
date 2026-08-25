#pragma once

#include "Entity.hpp"
#include <string_view>

#include <Hushgen.hpp>
#include <reflection/Type.hpp>
#include <serialization/Serialization.hpp>
#include <serialization/Deserialization.hpp>
#include <type_traits>

#if __has_include("RenderingSystemAPI.hushgen.hpp") && !defined(HUSH_HEADER_PARSING)
#include "RenderingSystemAPI.hushgen.hpp"
#endif

namespace Hush
{
	/// @brief Scripting-facing API of the rendering system as a component.
	struct [[hush::export]] [[hush::reflect]] RenderingSystemAPI
	{
		HUSH_GENERATED_BODY
	public:
		/// @brief Loads a cooked mesh asset and creates an entity that renders it.
		/// @param virtualPath Virtual path to the cooked mesh asset.
		/// @return Id of the created entity, or Entity::INVALID_ENTITY_ID on failure.
		[[hush::export]]
		Entity::EntityId (*instantiateMeshEntities)(const char* virtualPath, void* instance);

		[[hush::export]]
		void *instance;
	};
} // namespace Hush
