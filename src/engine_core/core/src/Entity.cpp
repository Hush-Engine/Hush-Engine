/*! \file Entity.cpp
	\author Alan Ramirez
	\date 2025-01-22
	\brief Scene entity
*/
#include "Entity.hpp"
#include "Assertions.hpp"
#include "EcsTerms.hpp"
#include "Logger.hpp"
#include "Scene.hpp"

#include <cstdint>
#include <flecs.h>
#include <flecs/addons/flecs_c.h>
#include <flecs/private/api_types.h>

void *Hush::ComponentRef::GetDataRaw()
{
	auto *ref = reinterpret_cast<ecs_ref_t *>(&this->m_refInternal);
	const auto *world = reinterpret_cast<const ecs_world_t *>(this->m_world);
	return ecs_ref_get_id(world, ref, ref->id);
}

const void *Hush::ComponentRef::GetDataRaw() const
{
	auto *ref = reinterpret_cast<ecs_ref_t *>(&this->m_refInternal);
	const auto *world = reinterpret_cast<const ecs_world_t *>(this->m_world);
	return ecs_ref_get_id(world, ref, ref->id);
}

Hush::Entity::EntityId Hush::ComponentRef::GetComponentId() const
{
	const auto *ref = reinterpret_cast<const ecs_ref_t *>(&this->m_refInternal);
	return ref->id;
}

Hush::Entity::EntityId Hush::Entity::RegisterComponentRaw(const ComponentTraits::ComponentInfo &desc) const
{
	return m_ownerScene->RegisterComponentRaw(desc);
}

void Hush::Entity::NotifyComponentModifiedRaw(Entity::EntityId componentId)
{
	auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());
	ecs_modified_id(world, this->m_entityId, componentId);
}

Hush::ComponentRef Hush::Entity::CreateComponentReferenceRaw(EntityId componentId)
{
	auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());
	ecs_ref_t ref = ecs_ref_init_id(world, this->m_entityId, componentId);
	static_assert(sizeof(ecs_ref_t) == ECS_REF_SIZE, "Reference size does not match to our internal usage!");

	ComponentRef publicRef{};

	auto *refInternalPtr = reinterpret_cast<ecs_ref_t *>(&publicRef.m_refInternal);
	*refInternalPtr = ref;
	publicRef.m_world = world;
	return publicRef;
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

void *Hush::Entity::EmplaceComponentRaw(EntityId componentId, size_t componentSize, bool &isNew)
{
	auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());

	void *component = ecs_emplace_id(world, m_entityId, componentId, componentSize, &isNew);

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

void Hush::Entity::SetComponentActiveRaw(EntityId componentId, bool active)
{
	auto *world = static_cast<ecs_world_t *>(this->m_ownerScene->GetWorld());
	// TODO: Check if we can get rid of this if, it's probably safer to keep, but yk
	if (!ecs_has_id(world, this->GetId(), componentId))
	{
		return;
	}
	ecs_enable_id(world, this->GetId(), componentId, active);
}

void Hush::Entity::Destroy(Entity &&entity)
{
	Scene *scene = entity.m_ownerScene;

	scene->DestroyEntity(std::move(entity));
}

void Hush::Entity::SetParent(const Entity &parent)
{
	(void)parent;
	LogError("Set Parent Not Yet Implemented");
}

void Hush::Entity::AddChild(const Entity &child)
{
	auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());
	ecs_add_pair(world, child.GetId(), EcsTerms::CHILD_OF, this->GetId());
}

Hush::Entity Hush::Entity::GetParent() const
{
	auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());
	Entity::EntityId parentId = ecs_get_parent(world, this->m_entityId);
	return Entity{this->m_ownerScene, parentId};
}

void Hush::Entity::EachChild(std::function<void(Entity &)> func) const
{
	auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());
	ecs_iter_t it = ecs_each_id(world, ecs_pair(EcsTerms::CHILD_OF, this->m_entityId));
	while (ecs_children_next(&it))
	{
		for (int32_t i = 0; i < it.count; i++)
		{
			Entity ent{this->m_ownerScene, it.entities[i]};
			func(ent);
		}
	}
}

Hush::Entity Hush::Entity::GetChildAt(int32_t index) const
{
	auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());
	ecs_iter_t it = ecs_each_id(world, ecs_pair(EcsTerms::CHILD_OF, this->m_entityId));
	if (!ecs_children_next(&it) || it.count < 1)
	{
		return Entity{this->m_ownerScene, INVALID_ENTITY_ID};
	}
	return Entity{this->m_ownerScene, it.entities[index]};
}

int32_t Hush::Entity::GetChildCount() const
{
	auto *world = static_cast<ecs_world_t *>(m_ownerScene->GetWorld());
	ecs_iter_t it = ecs_each_id(world, ecs_pair(EcsTerms::CHILD_OF, this->m_entityId));
	ecs_children_next(&it);
	return it.count;
}

void Hush::Entity::AddRelationship(const Entity &relationship, const Entity &target)
{
	HUSH_ASSERT(this->m_ownerScene == relationship.m_ownerScene, "Relationship entity must belong to the same scene");
	HUSH_ASSERT(this->m_ownerScene == target.m_ownerScene, "Target entity must belong to the same scene");
	auto *world = static_cast<ecs_world_t *>(this->m_ownerScene->GetWorld());

	ecs_add_pair(world, this->m_entityId, relationship.m_entityId, target.m_entityId);
}

Hush::Entity::EntityId Hush::Entity::GetId() const
{
	return m_entityId;
}

bool Hush::Entity::IsAlive() const
{
	auto *world = static_cast<ecs_world_t *>(this->GetSceneWorld());
	return world != nullptr && ecs_is_alive(world, this->m_entityId);
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

std::uint64_t Hush::Entity::GetSceneUniqueId() const
{
	return m_ownerScene->GetUniqueId();
}
