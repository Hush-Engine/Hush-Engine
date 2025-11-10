#include "QueryBuilder.hpp"
#include "Assertions.hpp"
#include "EcsTerms.hpp"
#include <flecs.h>

void Hush::impl::QueryBuilderImpl::WithRelationship(std::byte *queryDesc, uint8_t *termCountRef,
													const Entity &relationship)
{
	HUSH_ASSERT(queryDesc != nullptr, "Unable to build query, descriptor is null!");
	auto *desc = reinterpret_cast<ecs_query_desc_t *>(queryDesc);
	ecs_term_t relTerm{.id = ecs_pair(relationship.GetId(), EcsTerms::WILDCARD)};
	desc->terms[*termCountRef] = relTerm;
	(*termCountRef)++;
}

void Hush::impl::QueryBuilderImpl::WithRelationship(std::byte *queryDesc, uint8_t *termCountRef,
													const Entity &relationship, const Entity &target)
{
	HUSH_ASSERT(queryDesc != nullptr, "Unable to build query, descriptor is null!");
	auto *desc = reinterpret_cast<ecs_query_desc_t *>(queryDesc);

	ecs_term_t relTerm{.id = ecs_pair(relationship.GetId(), target.GetId())};
	desc->terms[*termCountRef] = relTerm;
	(*termCountRef)++;
}

void Hush::impl::QueryBuilderImpl::InitDescriptor(std::byte *queryDesc, std::span<Entity::EntityId> components)
{
	auto *desc = reinterpret_cast<ecs_query_desc_t *>(queryDesc);

	// Copy the components to the query description
	for (std::uint32_t i = 0; i < components.size(); ++i)
	{
		desc->terms[i].id = components[i];
	}
}

void *Hush::impl::QueryBuilderImpl::InitQuery(void *world, const std::byte *queryDesc)
{
	HUSH_ASSERT(world != nullptr, "Unable to build query for a null world!");
	HUSH_ASSERT(queryDesc != nullptr, "Unable to build query, descriptor is null!");
	const auto *desc = reinterpret_cast<const ecs_query_desc_t *>(queryDesc);
	auto *worldInterpreted = reinterpret_cast<ecs_world_t *>(world);
	return ecs_query_init(worldInterpreted, desc);
}
