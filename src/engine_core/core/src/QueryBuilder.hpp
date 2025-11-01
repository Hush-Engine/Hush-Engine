/*! \file QueryBuilder.hpp
	\author Kyn21kx
	\date 2025-10-31
	\brief Query builder implementation
*/

#pragma once
#include "Entity.hpp"
#include <array>
#include <cstddef>

namespace Hush {

	namespace impl::QueryBuilderImpl {
		// Unsafe implementations... TODO: Maybe use std::span?

		void WithRelationship(std::byte* queryDesc, uint8_t* termCountRef, const Entity& relationship);
		
		void WithRelationship(std::byte* queryDesc, uint8_t* termCountRef, const Entity& relationship, const Entity& target);

		void* InitQuery(void* world, const std::byte* queryDesc);
		
	}
	
	template <typename... Components>
	class QueryBuilder {
	public:
		QueryBuilder(void* rawEcsWorld) {
			this->m_world = rawEcsWorld;
		}

		/**
		@brief Adds a relationship filter to the query
		@param relationship Relationship to filter entities by
		@details Matched entities will be related to any other entity by the specified relationship paramter,
		using this function is equivalent to WithRelationship(relationship, EcsTerms::WILDCARD)
		*/
		inline QueryBuilder& WithRelationship(const Entity& relationship) {
			impl::QueryBuilderImpl::WithRelationship(this->m_opaqueDesc.data(), relationship);
			return *this;
		}
		
		inline QueryBuilder& WithRelationship(const Entity& relationship, const Entity& target) {
			impl::QueryBuilderImpl::WithRelationship(this->m_opaqueDesc.data(), relationship, target);
			return *this;
		}
		
		Query<Components...> Build() {
			void* initializedQuery = impl::QueryBuilderImpl::InitQuery(this->m_world, this->m_opaqueDesc.data());
			return {};
		}

	private:
		static constexpr size_t COMP_COUNT = sizeof...(Components);
		static constexpr size_t DESC_ALIGN = 8;
		static constexpr size_t DESC_SIZE = 2440;
		
		alignas(DESC_ALIGN) std::array<std::byte, DESC_SIZE> m_opaqueDesc{};
		void* m_world = nullptr;
		uint8_t m_termCount = COMP_COUNT;
	};
	
}


