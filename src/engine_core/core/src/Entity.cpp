/*! \file Entity.cpp
	\author Alan Ramirez
	\date 2025-01-22
	\brief Scene entity
*/
#include "Entity.hpp"
#include "Scene.hpp"

#include <flecs.h>

Hush::Entity::EntityId Hush::Entity::RegisterComponentRaw(const ComponentTraits::ComponentInfo &desc) const
{
	return m_ownerScene->RegisterComponentRaw(desc);
}

void *Hush::Entity::AddComponentRaw(const EntityId componentId)
{
	auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());

	ecs_add_id(world, m_entityId, componentId);

	return ecs_get_mut_id(world, m_entityId, componentId);
}

void *Hush::Entity::GetComponentRaw(EntityId componentId)
{
	auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());

	return ecs_get_mut_id(world, m_entityId, componentId);
}

void *Hush::Entity::GetComponentRaw(EntityId componentId) const
{
	auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());

	return ecs_get_mut_id(world, m_entityId, componentId);
}

bool Hush::Entity::HasComponentRaw(EntityId componentId)
{
	auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());

	return ecs_has_id(world, m_entityId, componentId);
}

void *Hush::Entity::EmplaceComponentRaw(EntityId componentId, bool &is_new)
{
	auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());

	void *component = ecs_emplace_id(world, m_entityId, componentId, &is_new);

	return component;
}

bool Hush::Entity::RemoveComponentRaw(EntityId componentId)
{
	auto world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());

	if (ecs_has_id(world, m_entityId, componentId))
	{
		ecs_remove_id(world, m_entityId, componentId);
		return true;
	}

	return false;
}

void Hush::Entity::Destroy(Entity &&entity)
{
	Scene *scene = entity.m_ownerScene;

	scene->DestroyEntity(std::move(entity));
}

Hush::Entity::EntityId Hush::Entity::GetId() const
{
	return m_entityId;
}

// std::optional<std::string_view> Hush::Entity::GetName() const
// {
// 	auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());

// 	const char *name = ecs_get_name(world, m_entityId);

// 	if (name == nullptr)
// 	{
// 		return std::nullopt;
// 	}

// 	return std::string_view(name);
// }

void *Hush::Entity::GetSceneWorld() const
{
	return m_ownerScene->GetWorld();
}

bool Hush::Entity::IsComponentRegistered(EntityId componentId) const
{
	return ecs_has_id(static_cast<ecs_world_t *>(m_ownerScene->GetWorld()), m_entityId, componentId);
}

Hush::Entity::EntityId Hush::Entity::InternalRegisterCppComponent(
	ComponentTraits::detail::EEntityRegisterStatus registerStatus, std::uint64_t *id,
	const ComponentTraits::ComponentInfo &desc) const
{
	return m_ownerScene->InternalRegisterCppComponent(registerStatus, id, desc);
}

std::optional<Hush::Entity::EntityId> Hush::Entity::InternalCachedComponentId(const std::string_view name) const
{
	return m_ownerScene->GetRegisteredComponentId(name);
}
