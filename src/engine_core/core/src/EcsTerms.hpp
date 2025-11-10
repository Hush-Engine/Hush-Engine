/*! \file EcsTerms.hpp
	\author Kyn21kx
	\date 2025-10-31
	\brief Constant terms for ECS operations
*/

#pragma once

#include "Entity.hpp"

namespace Hush::EcsTerms
{
	extern const Entity::EntityId WILDCARD;
	extern const Entity::EntityId ANY;
	extern const Entity::EntityId CHILD_OF;
} // namespace Hush::EcsTerms
