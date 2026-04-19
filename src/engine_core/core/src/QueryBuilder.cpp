#include "QueryBuilder.hpp"
#include "Assertions.hpp"
#include "EcsTerms.hpp"
#include "Query.hpp"
#include <flecs.h>
#include "Scene.hpp"

constexpr uint8_t MAX_TERM_COUNT = 32;

uint8_t *Hush::OpaqueQueryDescriptor::data() noexcept
{
	return reinterpret_cast<uint8_t *>(this->m_opaqueDesc.data());
}

void Hush::impl::QueryBuilderImpl::WithRelationship(uint8_t *queryDesc, uint8_t *termCountRef,
													const Entity &relationship)
{
	HUSH_ASSERT(queryDesc != nullptr, "Unable to build query, descriptor is null!");
	auto *desc = reinterpret_cast<ecs_query_desc_t *>(queryDesc);
	ecs_term_t relTerm{.id = ecs_pair(relationship.GetId(), EcsTerms::WILDCARD)};
	desc->terms[*termCountRef] = relTerm;
	(*termCountRef)++;
}

void Hush::impl::QueryBuilderImpl::WithRelationship(uint8_t *queryDesc, uint8_t *termCountRef,
													const Entity &relationship, const Entity &target)
{
	HUSH_ASSERT(queryDesc != nullptr, "Unable to build query, descriptor is null!");
	HUSH_ASSERT(termCountRef != nullptr, "Cannot add term with a null count reference!");
	HUSH_COND_FAIL_MSG(*termCountRef < MAX_TERM_COUNT,
					   "Maximum amount of query terms exceeded, you can only query for {} terms at once!",
					   MAX_TERM_COUNT);
	auto *desc = reinterpret_cast<ecs_query_desc_t *>(queryDesc);

	ecs_term_t relTerm{.id = ecs_pair(relationship.GetId(), target.GetId())};
	desc->terms[*termCountRef] = relTerm;
	(*termCountRef)++;
}

void Hush::impl::QueryBuilderImpl::WithTerm(uint8_t *queryDesc, uint8_t *termCountRef, Entity::EntityId term)
{
	HUSH_ASSERT(queryDesc != nullptr, "Unable to build query, descriptor is null!");
	HUSH_ASSERT(termCountRef != nullptr, "Cannot add term with a null count reference!");
	HUSH_COND_FAIL_MSG(*termCountRef < MAX_TERM_COUNT,
					   "Maximum amount of query terms exceeded, you can only query for {} terms at once!",
					   MAX_TERM_COUNT);

	auto *desc = reinterpret_cast<ecs_query_desc_t *>(queryDesc);

	ecs_term_t builtTerm{.id = term};

	desc->terms[*termCountRef] = builtTerm;
	(*termCountRef)++;
}

void Hush::impl::QueryBuilderImpl::InitDescriptor(uint8_t *queryDesc, std::span<Entity::EntityId> components)
{
	auto *desc = reinterpret_cast<ecs_query_desc_t *>(queryDesc);

	// Copy the components to the query description
	for (std::uint32_t i = 0; i < components.size(); ++i)
	{
		desc->terms[i].id = components[i];
	}
}

void Hush::impl::QueryBuilderImpl::Without(uint8_t *queryDesc, uint8_t *termCountRef, Entity::EntityId term)
{
	HUSH_ASSERT(queryDesc != nullptr, "Unable to build query, descriptor is null!");
	HUSH_ASSERT(termCountRef != nullptr, "Cannot add term with a null count reference!");
	HUSH_COND_FAIL_MSG(*termCountRef < MAX_TERM_COUNT,
					   "Maximum amount of query terms exceeded, you can only query for {} terms at once!",
					   MAX_TERM_COUNT);

	auto *desc = reinterpret_cast<ecs_query_desc_t *>(queryDesc);

	ecs_term_t builtTerm{.id = term, .oper = EcsNot};

	desc->terms[*termCountRef] = builtTerm;
	(*termCountRef)++;
}

void Hush::impl::QueryBuilderImpl::WithOptional(uint8_t *queryDesc, uint8_t *termCountRef, Entity::EntityId term)
{
	HUSH_ASSERT(queryDesc != nullptr, "Unable to build query, descriptor is null!");
	HUSH_ASSERT(termCountRef != nullptr, "Cannot add term with a null count reference!");
	HUSH_COND_FAIL_MSG(*termCountRef < MAX_TERM_COUNT,
					   "Maximum amount of query terms exceeded, you can only query for {} terms at once!",
					   MAX_TERM_COUNT);

	auto *desc = reinterpret_cast<ecs_query_desc_t *>(queryDesc);

	ecs_term_t builtTerm{.id = term, .oper = EcsOptional};

	desc->terms[*termCountRef] = builtTerm;
	(*termCountRef)++;
}

Hush::RawQuery Hush::impl::QueryBuilderImpl::InitQuery(Scene *scene, const uint8_t *queryDesc)
{
	HUSH_ASSERT(scene != nullptr, "Unable to build query for a null scene!");
	HUSH_ASSERT(queryDesc != nullptr, "Unable to build query, descriptor is null!");
	const auto *desc = reinterpret_cast<const ecs_query_desc_t *>(queryDesc);
	auto *worldInterpreted = reinterpret_cast<ecs_world_t *>(scene->GetWorld());
	void *queryData = ecs_query_init(worldInterpreted, desc);
	return RawQuery{scene, queryData};
}
