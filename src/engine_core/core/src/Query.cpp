/*! \file Query.cpp
    \author Alan Ramirez
    \date 2025-02-01
    \brief Query implementation
*/

#include "Query.hpp"

#include <Scene.hpp>
#include <flecs.h>

Hush::RawQuery::RawQuery(Scene *scene, void *query)
    : m_query(query),
      m_scene(scene)
{
}

bool Hush::RawQuery::QueryIterator::Next()
{
    if (m_hasBeenDestroyed)
    {
        return false;
    }

    auto *queryIter = reinterpret_cast<ecs_iter_t *>(m_iterData.data());

    const bool hasNext = ecs_query_next(queryIter);

    if (!hasNext)
    {
        m_hasBeenDestroyed = true;
    }

    return hasNext;
}

void Hush::RawQuery::QueryIterator::Skip()
{
    if (m_hasBeenDestroyed)
    {
        return;
    }

    auto *queryIter = reinterpret_cast<ecs_iter_t *>(m_iterData.data());

    ecs_iter_skip(queryIter);
}

std::size_t Hush::RawQuery::QueryIterator::Size() const
{
    auto *queryIter = reinterpret_cast<const ecs_iter_t *>(m_iterData.data());

    return queryIter->count;
}

void *const Hush::RawQuery::QueryIterator::GetComponentAt(std::int8_t index, std::size_t size) const
{
    auto *queryIter = reinterpret_cast<const ecs_iter_t *>(m_iterData.data());

    return ecs_field_w_size(queryIter, size, index);
}

std::uint64_t Hush::RawQuery::QueryIterator::GetEntityAt(std::size_t index) const
{
    auto *queryIter = reinterpret_cast<const ecs_iter_t *>(m_iterData.data());

    return queryIter->entities[index];
}

Hush::RawQuery::QueryIterator::QueryIterator(QueryIterator &&rhs) noexcept
    : m_iterData(std::move(rhs.m_iterData)),
      m_hasBeenDestroyed(std::exchange(rhs.m_hasBeenDestroyed, true))
{
}

Hush::RawQuery::QueryIterator &Hush::RawQuery::QueryIterator::operator=(QueryIterator &&rhs) noexcept
{
    if (this != &rhs)
    {
        m_iterData = std::move(rhs.m_iterData);
        m_hasBeenDestroyed = std::exchange(rhs.m_hasBeenDestroyed, true);
    }

    return *this;
}

Hush::RawQuery::QueryIterator::~QueryIterator()
{
    if (m_hasBeenDestroyed)
    {
        return;
    }

    auto *queryIter = reinterpret_cast<ecs_iter_t *>(m_iterData.data());

    ecs_iter_fini(queryIter);
}

Hush::RawQuery::RawQuery(RawQuery &&rhs) noexcept
    : m_query(std::exchange(rhs.m_query, nullptr)),
      m_scene(rhs.m_scene)
{
}

Hush::RawQuery &Hush::RawQuery::operator=(RawQuery &&rhs) noexcept
{
    if (this != &rhs)
    {
        m_query = std::exchange(rhs.m_query, nullptr);
        m_scene = rhs.m_scene;
    }

    return *this;
}

Hush::RawQuery::~RawQuery() noexcept
{
    auto query = static_cast<ecs_query_t *>(m_query);

    if (query == nullptr)
    {
        return;
    }

    ecs_query_fini(query);
}

Hush::RawQuery::QueryIterator Hush::RawQuery::GetIterator()
{
    auto world = static_cast<ecs_world_t *>(m_scene->GetWorld());

    auto queryIter = QueryIterator(m_scene);

    ::new (queryIter.m_iterData.data()) ecs_iter_t(ecs_query_iter(world, static_cast<ecs_query_t *>(m_query)));

    return queryIter;
}

Hush::impl::QueryImpl::EntityId Hush::impl::QueryImpl::InternalRegisterCppComponent(
    ComponentTraits::detail::EEntityRegisterStatus registerStatus, std::uint64_t *id,
    const ComponentTraits::ComponentInfo &desc)
{
    Scene *scene = m_rawQuery.GetScene();

    return scene->InternalRegisterCppComponent(registerStatus, id, desc);
}