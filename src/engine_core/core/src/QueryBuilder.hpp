/*! \file QueryBuilder.hpp
	\author Kyn21kx
	\date 2025-10-31
	\brief Query builder implementation
*/

#pragma once
#include "Entity.hpp"
#include "Query.hpp"
#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace Hush
{

	class Scene;

	namespace impl::QueryBuilderImpl
	{
		// Unsafe implementations... TODO: Maybe use std::span?

		[[hush::export]]
		void WithRelationship(uint8_t *queryDesc, uint8_t *termCountRef, const Entity &relationship);

		[[hush::export]]
		void WithRelationship(uint8_t *queryDesc, uint8_t *termCountRef, const Entity &relationship,
							  const Entity &target);

		[[hush::export]]
		void WithTerm(uint8_t *queryDesc, uint8_t *termCountRef, Entity::EntityId term);

		[[hush::export]]
		void InitDescriptor(uint8_t *queryDesc, std::span<Entity::EntityId> components);

		[[hush::export]]
		RawQuery InitQuery(Scene *scene, const uint8_t *queryDesc);

	} // namespace impl::QueryBuilderImpl

	constexpr size_t DESC_ALIGN = 8;
	constexpr size_t DESC_SIZE = 2440;

	struct [[hush::export]] OpaqueQueryDescriptor
	{
	public:
		// NOLINTBEGIN
		[[hush::export]]
		uint8_t *data() noexcept;
		// NOLINTEND

	private:
		alignas(DESC_ALIGN) std::array<std::byte, DESC_SIZE> m_opaqueDesc{};
	};

	template <typename... Components>
	class QueryBuilder
	{
	public:
		/**
		@brief Adds a relationship filter to the query
		@param relationship Relationship to filter entities by
		@details Matched entities will be related to any other entity by the specified relationship paramter,
		using this function is equivalent to WithRelationship(relationship, EcsTerms::WILDCARD)
		*/
		inline QueryBuilder &WithRelationship(const Entity &relationship)
		{
			impl::QueryBuilderImpl::WithRelationship(this->m_opaqueDesc.data(), &this->m_termCount, relationship);
			return *this;
		}

		inline QueryBuilder &WithRelationship(const Entity &relationship, const Entity &target)
		{
			impl::QueryBuilderImpl::WithRelationship(this->m_opaqueDesc.data(), &this->m_termCount, relationship,
													 target);
			return *this;
		}

		Query<Components...> Build()
		{
			RawQuery initializedQuery = impl::QueryBuilderImpl::InitQuery(this->m_scene, this->m_opaqueDesc.data());
			return Query<Components...>(initializedQuery);
		}

	private:
		friend class Scene;

		QueryBuilder(Scene *scene, void *rawEcsWorld, std::span<Entity::EntityId> components)
		{
			this->m_world = rawEcsWorld;
			this->m_scene = scene;
			impl::QueryBuilderImpl::InitDescriptor(this->m_opaqueDesc.data(), components);
		}

		static constexpr size_t COMP_COUNT = sizeof...(Components);
		static constexpr size_t DESC_ALIGN = 8;
		static constexpr size_t DESC_SIZE = 2440;

		OpaqueQueryDescriptor m_opaqueDesc{};
		// We need the world apart from the scene to avoid including it as a full on header on this compilation unit
		void *m_world = nullptr;
		Scene *m_scene = nullptr;
		uint8_t m_termCount = COMP_COUNT;
	};

} // namespace Hush
