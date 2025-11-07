/*! \file Entity.cpp
	\author Alan Ramirez
	\date 2025-01-22
	\brief Scene entity
*/
#include "Entity.hpp"
#include "EcsTerms.hpp"
#include "Logger.hpp"
#include "Scene.hpp"

#include <cstdint>
#include <flecs.h>
#include <flecs/addons/flecs_c.h>

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

void *Hush::Entity::EmplaceComponentRaw(EntityId componentId, bool &isNew)
{
	auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());

	void *component = ecs_emplace_id(world, m_entityId, componentId, &isNew);

	return component;
}

bool Hush::Entity::RemoveComponentRaw(EntityId componentId)
{
	auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());

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

void Hush::Entity::SetParent(const Entity& parent) {
	(void)parent;
	LogError("Set Parent Not Yet Implemented");
}

void Hush::Entity::AddChild(const Entity& child) {
	auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());
	ecs_add_pair(world, child.GetId(), EcsTerms::CHILD_OF, this->GetId());
}

Hush::Entity Hush::Entity::GetParent() const {
	auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());
	Entity::EntityId parentId = ecs_get_parent(world, this->m_entityId);
	return Entity {this->m_ownerScene, parentId};
}


void Hush::Entity::EachChild(std::function<void(Entity&)> func) const {
	auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());
	ecs_iter_t it = ecs_each_id(world, ecs_pair(EcsTerms::CHILD_OF, this->m_entityId));
	while (ecs_children_next(&it)) {
		for (int32_t i = 0; i < it.count; i++) {
			Entity ent {this->m_ownerScene, it.entities[i]};
			func(ent);
		}
	}
}

Hush::Entity Hush::Entity::GetChildAt(int32_t index) const {
	auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());
	ecs_iter_t it = ecs_each_id(world, ecs_pair(EcsTerms::CHILD_OF, this->m_entityId));
	if (!ecs_children_next(&it) || it.count < 1) {
		return Entity {this->m_ownerScene, INVALID_ENTITY_ID};
	}
	return Entity {this->m_ownerScene, it.entities[index]};
}

int32_t Hush::Entity::GetChildCount() const {
	auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());
	ecs_iter_t it = ecs_each_id(world, ecs_pair(EcsTerms::CHILD_OF, this->m_entityId));
	ecs_children_next(&it);
	return it.count;
}

Hush::Entity::EntityId Hush::Entity::GetId() const
{
	return m_entityId;
}

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
