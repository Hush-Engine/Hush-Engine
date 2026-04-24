/*! \file Scene.cpp
	\author Alan Ramirez
	\date 2025-01-20
	\brief Scene implementation
*/

#include "Scene.hpp"
#include "ISystem.hpp"
#include "Logger.hpp"
#include "utils/ParallelUtils.hpp"
#include <flecs.h>
#include <flecs/addons/flecs_c.h>
#include "Profiling.hpp"

constexpr std::size_t DEFAULT_SYSTEMS_CAPACITY = 128;

Hush::Scene::Scene(HushEngine *engine, Hush::Threading::Executors::ThreadPool *threadPool)
	: m_engine(engine),
	  m_world(ecs_init()),
	  m_threadPool(threadPool)
{
	// Reserve the buckets
	m_userSystems.reserve(DEFAULT_SYSTEMS_CAPACITY);
}

Hush::Scene::~Scene()
{
	ecs_fini(static_cast<ecs_world_t *>(m_world));
}

void Hush::Scene::Init()
{
#ifndef HUSH_PLATFORM_EMSCRIPTEN
	ZoneScoped;
#endif
	for (const std::vector<ISystem *> &systemBucket : m_systems)
	{
		Threading::Wait(Threading::ParallelFor(m_threadPool, systemBucket.begin(), systemBucket.end(),
											   [](ISystem *system) { system->Init(); }));
	}

	// TODO: Group user systems into buckets
	ScriptingSystemInterface::CallSystemInit_t initFunc = this->m_scriptingInterface->initFunction;
	Threading::Wait(
		Threading::ParallelFor(m_threadPool, this->m_scriptingSystems.begin(), this->m_scriptingSystems.end(),
							   [initFunc](uintptr_t system) { initFunc(reinterpret_cast<void *>(system)); }));

	this->m_isInitialized = true;
}

void Hush::Scene::Update(float delta)
{
#ifndef HUSH_PLATFORM_EMSCRIPTEN
	ZoneScoped;
#endif
	for (const std::vector<ISystem *> &systemBucket : m_systems)
	{
		Threading::Wait(
			Threading::ParallelFor(m_threadPool, systemBucket.begin(), systemBucket.end(), [delta](ISystem *system) {
				// Call the update method for each system
				system->OnUpdate(delta);
			}));
	}

	ScriptingSystemInterface::CallSystemOnUpdate_t updateFunc = this->m_scriptingInterface->updateFunction;
	// TODO: Sort in threading
	for (uintptr_t system : this->m_scriptingSystems)
	{
		updateFunc(reinterpret_cast<void *>(system), delta);
	}
}

void Hush::Scene::FixedUpdate(float delta)
{
#ifndef HUSH_PLATFORM_EMSCRIPTEN
	ZoneScoped;
#endif
	for (const std::vector<ISystem *> &systemBucket : m_systems)
	{
		Threading::Wait(
			Threading::ParallelFor(m_threadPool, systemBucket.begin(), systemBucket.end(), [delta](ISystem *system) {
				// Call the fixed update method for each system
				system->OnFixedUpdate(delta);
			}));
	}

	ScriptingSystemInterface::CallSystemOnFixedUpdate_t fixedUpdateFunc =
		this->m_scriptingInterface->fixedUpdateFunction;
	// TODO: Sort in threading
	for (uintptr_t system : this->m_scriptingSystems)
	{
		fixedUpdateFunc(reinterpret_cast<void *>(system), delta);
	}
}

void Hush::Scene::PreRender()
{
#ifndef HUSH_PLATFORM_EMSCRIPTEN
	ZoneScoped;
#endif
	for (const std::vector<ISystem *> &systemBucket : m_systems)
	{
		Threading::Wait(Threading::ParallelFor(m_threadPool, systemBucket.begin(), systemBucket.end(),
											   [](ISystem *system) { system->OnPreRender(); }));
	}

	ScriptingSystemInterface::CallSystemOnPreRender_t preRenderFunc = this->m_scriptingInterface->preRenderFunction;
	// TODO: Sort in threading
	for (uintptr_t system : this->m_scriptingSystems)
	{
		preRenderFunc(reinterpret_cast<void *>(system));
	}
}

void Hush::Scene::Render()
{
#ifndef HUSH_PLATFORM_EMSCRIPTEN
	ZoneScoped;
#endif
	for (const std::vector<ISystem *> &systemBucket : m_systems)
	{
		Threading::Wait(Threading::ParallelFor(m_threadPool, systemBucket.begin(), systemBucket.end(),
											   [](ISystem *system) { system->OnRender(); }));
	}

	ScriptingSystemInterface::CallSystemOnRender_t renderFunc = this->m_scriptingInterface->renderFunction;
	// TODO: Sort in threading
	for (uintptr_t system : this->m_scriptingSystems)
	{
		renderFunc(reinterpret_cast<void *>(system));
	}
}

void Hush::Scene::PostRender()
{
#ifndef HUSH_PLATFORM_EMSCRIPTEN
	ZoneScoped;
#endif
	for (const std::vector<ISystem *> &systemBucket : m_systems)
	{
		Threading::Wait(Threading::ParallelFor(m_threadPool, systemBucket.begin(), systemBucket.end(),
											   [](ISystem *system) { system->OnPostRender(); }));
	}

	ScriptingSystemInterface::CallSystemOnPostRender_t postRender = this->m_scriptingInterface->postRenderFunction;
	// TODO: Sort in threading
	for (uintptr_t system : this->m_scriptingSystems)
	{
		postRender(reinterpret_cast<void *>(system));
	}
}

void Hush::Scene::Shutdown()
{
#ifndef HUSH_PLATFORM_EMSCRIPTEN
	ZoneScoped;
#endif
	for (const std::vector<ISystem *> &systemBucket : m_systems)
	{
		Threading::Wait(Threading::ParallelFor(m_threadPool, systemBucket.begin(), systemBucket.end(),
											   [](ISystem *system) { system->OnShutdown(); }));
	}

	ScriptingSystemInterface::CallSystemOnShutdown_t shutdownFunc = this->m_scriptingInterface->shutdownFunction;
	// TODO: Sort in threading
	for (uintptr_t system : this->m_scriptingSystems)
	{
		shutdownFunc(reinterpret_cast<void *>(system));
	}
}

void Hush::Scene::RemoveSystem(std::string_view name)
{
	// Find the system
	auto it = std::ranges::find_if(
		m_userSystems, [name](const std::unique_ptr<ISystem> &system) { return system->GetName() == name; });

	// If the system was found, remove it
	if (it != m_userSystems.end())
	{
		m_userSystems.erase(it);
	}

	// Sort the systems again
	SortSystems();
}

Hush::Entity Hush::Scene::CreateEntity()
{
	auto *world = static_cast<ecs_world_t *>(m_world);

	const Entity::EntityId entityId = ecs_new(world);

	return Entity{this, entityId};
}

Hush::Entity Hush::Scene::CreateEntityWithName(std::string_view name)
{
	auto *world = static_cast<ecs_world_t *>(m_world);
	const Entity::EntityId entityId = ecs_new(world);

	Entity result{this, entityId};
	result.EmplaceComponent<Entity::Name>(name);
	return result;
}

void Hush::Scene::DestroyEntity(Entity &&entity)
{
	auto entityToDestroy = std::move(entity);
	auto *world = static_cast<ecs_world_t *>(m_world);

	ecs_delete(world, entityToDestroy.GetId());
}

void Hush::Scene::DestroyEntity(Entity &entity)
{
	auto *world = static_cast<ecs_world_t *>(m_world);

	ecs_delete(world, entity.GetId());
}

std::optional<std::uint64_t> Hush::Scene::GetRegisteredComponentId(std::string_view name)
{
	std::shared_lock lock(m_registeredEntitiesMutex);
	const auto entityIt = m_registeredEntities.find(name.data());

	if (entityIt != m_registeredEntities.end())
	{
		return entityIt->second;
	}

	// Component not found.
	return std::nullopt;
}

void Hush::Scene::RegisterComponentId(std::string_view name, Entity::EntityId id)
{
	std::unique_lock lock(m_registeredEntitiesMutex);
	m_registeredEntities.insert_or_assign(name.data(), id);
}

std::optional<Hush::Entity> Hush::Scene::EntityFromId(EntityId id)
{
	auto *world = static_cast<ecs_world_t *>(m_world);
	if (!ecs_is_valid(world, id))
	{
		return std::nullopt;
	}
	return Entity{this, id};
}

Hush::Entity Hush::Scene::EntityFromIdUnchecked(EntityId id)
{
	return Entity{this, id};
}

Hush::Entity::EntityId Hush::Scene::RegisterComponentRaw(const ComponentTraits::ComponentInfo &desc) const
{
	struct ComponentInfo
	{
		std::size_t size{};
		std::size_t alignment{};
		std::string name;
		ComponentTraits::ComponentOps ops{};
		ComponentTraits::EComponentOpsFlags opsFlags{ComponentTraits::EComponentOpsFlags::None};

		void *userCtx{};
		void (*userCtxFree)(void *){};
	};

	// TODO: Arena
	auto *componentInfo = new ComponentInfo();
	componentInfo->size = desc.size;
	componentInfo->alignment = desc.alignment;
	componentInfo->name = desc.name;
	componentInfo->ops = desc.ops;
	componentInfo->userCtx = desc.userCtx;
	componentInfo->userCtxFree = desc.userCtxFree;

	ecs_component_desc_t componentDesc = {};
	ecs_entity_desc_t associatedEntityDesc = {};

	associatedEntityDesc.name = desc.name;

	componentDesc.type.alignment = static_cast<ecs_size_t>(componentInfo->alignment);
	componentDesc.type.size = static_cast<ecs_size_t>(componentInfo->size);
	componentDesc.type.name = componentInfo->name.data();
	componentDesc.type.hooks.binding_ctx = componentInfo;

	auto *world = static_cast<ecs_world_t *>(const_cast<void *>(GetWorld()));
	componentDesc.entity = ecs_entity_init(world, &associatedEntityDesc);

	componentDesc.type.hooks.binding_ctx_free = [](void *ctx) {
		const auto *info = static_cast<ComponentInfo *>(ctx);
		if (info->userCtxFree != nullptr)
		{
			info->userCtxFree(info->userCtx);
		}
		delete info;
	};

	if (desc.ops.ctor != nullptr)
	{
		componentDesc.type.hooks.ctor = [](void *ptr, int32_t count, const ecs_type_info_t *type_info) {
			const auto *info = static_cast<ComponentInfo *>(type_info->hooks.binding_ctx);

			ComponentTraits::ComponentInfo componentDesc = {
				.size = info->size,
				.alignment = info->alignment,
				.name = info->name.c_str(),
				.ops = info->ops,
				.opsFlags = info->opsFlags,
				.userCtx = info->userCtx,
				.userCtxFree = info->userCtxFree,
			};

			info->ops.ctor(ptr, count, &componentDesc);
		};
	}

	if (desc.ops.dtor != nullptr)
	{
		componentDesc.type.hooks.dtor = [](void *ptr, int32_t count, const ecs_type_info_t *type_info) {
			const auto *info = static_cast<ComponentInfo *>(type_info->hooks.binding_ctx);

			ComponentTraits::ComponentInfo componentDesc = {
				.size = info->size,
				.alignment = info->alignment,
				.name = info->name.c_str(),
				.ops = info->ops,
				.opsFlags = info->opsFlags,
				.userCtx = info->userCtx,
				.userCtxFree = info->userCtxFree,
			};

			info->ops.dtor(ptr, count, &componentDesc);
		};
	}

	if (desc.ops.copy != nullptr)
	{
		componentDesc.type.hooks.copy = [](void *dst, const void *src, int32_t count,
										   const ecs_type_info_t *type_info) {
			const auto *info = static_cast<ComponentInfo *>(type_info->hooks.binding_ctx);

			ComponentTraits::ComponentInfo componentDesc = {
				.size = info->size,
				.alignment = info->alignment,
				.name = info->name.c_str(),
				.ops = info->ops,
				.opsFlags = info->opsFlags,
				.userCtx = info->userCtx,
				.userCtxFree = info->userCtxFree,
			};

			info->ops.copy(dst, src, count, &componentDesc);
		};
	}

	if (desc.ops.move != nullptr)
	{
		componentDesc.type.hooks.move = [](void *dst, void *src, int32_t count, const ecs_type_info_t *type_info) {
			const auto *info = static_cast<ComponentInfo *>(type_info->hooks.binding_ctx);

			ComponentTraits::ComponentInfo componentDesc = {
				.size = info->size,
				.alignment = info->alignment,
				.name = info->name.c_str(),
				.ops = info->ops,
				.opsFlags = info->opsFlags,
				.userCtx = info->userCtx,
				.userCtxFree = info->userCtxFree,
			};

			info->ops.move(dst, src, count, &componentDesc);
		};
	}

	if (desc.ops.copyCtor != nullptr)
	{
		componentDesc.type.hooks.copy_ctor = [](void *dst, const void *src, int32_t count,
											const ecs_type_info_t *type_info) {
			const auto *info = static_cast<ComponentInfo *>(type_info->hooks.binding_ctx);

			ComponentTraits::ComponentInfo componentDesc = {
				.size = info->size,
				.alignment = info->alignment,
				.name = info->name.c_str(),
				.ops = info->ops,
				.opsFlags = info->opsFlags,
				.userCtx = info->userCtx,
				.userCtxFree = info->userCtxFree,
			};

			info->ops.copyCtor(dst, src, count, &componentDesc);
		};
	}

	if (desc.ops.moveCtor != nullptr)
	{
		componentDesc.type.hooks.move_ctor = [](void *dst, void *src, int32_t count, const ecs_type_info_t *type_info) {
			const auto *info = static_cast<ComponentInfo *>(type_info->hooks.binding_ctx);

			ComponentTraits::ComponentInfo componentDesc = {
				.size = info->size,
				.alignment = info->alignment,
				.name = info->name.c_str(),
				.ops = info->ops,
				.opsFlags = info->opsFlags,
				.userCtx = info->userCtx,
				.userCtxFree = info->userCtxFree,
			};

			info->ops.moveCtor(dst, src, count, &componentDesc);
		};
	}

	if (desc.ops.moveDtor != nullptr)
	{
		componentDesc.type.hooks.move_dtor = [](void *dst, void *src, int32_t count, const ecs_type_info_t *type_info) {
			const auto *info = static_cast<ComponentInfo *>(type_info->hooks.binding_ctx);

			ComponentTraits::ComponentInfo componentDesc = {
				.size = info->size,
				.alignment = info->alignment,
				.name = info->name.c_str(),
				.ops = info->ops,
				.opsFlags = info->opsFlags,
				.userCtx = info->userCtx,
				.userCtxFree = info->userCtxFree,
			};

			info->ops.moveDtor(dst, src, count, &componentDesc);
		};
	}

	if (desc.ops.moveAssignDtor != nullptr)
	{
		componentDesc.type.hooks.move_dtor = [](void *dst, void *src, int32_t count, const ecs_type_info_t *type_info) {
			const auto *info = static_cast<ComponentInfo *>(type_info->hooks.binding_ctx);

			ComponentTraits::ComponentInfo componentDesc = {
				.size = info->size,
				.alignment = info->alignment,
				.name = info->name.c_str(),
				.ops = info->ops,
				.opsFlags = info->opsFlags,
				.userCtx = info->userCtx,
				.userCtxFree = info->userCtxFree,
			};

			info->ops.moveAssignDtor(dst, src, count, &componentDesc);
		};
	}

	componentDesc.type.hooks.flags = static_cast<std::uint32_t>(desc.opsFlags);

	// Register the component
	ecs_entity_t componentId = ecs_component_init(world, &componentDesc);

	// By default, all components should be able to be toggled on or off (for performance reasons)
	ecs_add_id(world, componentId, EcsCanToggle);

	LogFormat(ELogLevel::Info, "Registered component with name {} as ID: {}", desc.name, componentId);

	return componentId;
}

Hush::Entity::EntityId Hush::Scene::Lookup(std::string_view tag) const
{
	auto *world = static_cast<ecs_world_t *>(this->m_world);
	Entity::EntityId result = ecs_lookup(world, tag.data());
	return result;
}

Hush::RawQuery Hush::Scene::CreateRawQuery(std::span<Entity::EntityId> components, RawQuery::ECacheMode cacheMode)
{
	auto *const world = static_cast<ecs_world_t *>(m_world);

	ecs_query_desc_t queryDesc = {};
	// Copy the components to the query description
	for (std::uint32_t i = 0; i < components.size(); ++i)
	{
		queryDesc.terms[i].id = components[i];
	}

	queryDesc.cache_kind = static_cast<ecs_query_cache_kind_t>(cacheMode);

	ecs_query_t *query = ecs_query_init(world, &queryDesc);

	return RawQuery{this, query};
}

Hush::Entity::EntityId Hush::Scene::InternalRegisterCppComponent(
	ComponentTraits::detail::EEntityRegisterStatus registerStatus, std::uint64_t *id,
	const ComponentTraits::ComponentInfo &desc)
{
	// Slow path, the component is zero, which means: 1. It is not registered in this binary instance, or 2. It
	// has never been registered.
	if (registerStatus == ComponentTraits::detail::EEntityRegisterStatus::NotRegistered)
	{
		// First, check if the component is already registered in the scene.
		if (auto cachedComponentId = GetRegisteredComponentId(desc.name); cachedComponentId.has_value())
		{
			// Okay, already registered in the scene by another thread or translation unit.
			*id = *cachedComponentId;
		}
		else
		{
			// We need to register the component.
			*id = RegisterComponentRaw(desc);
			RegisterComponentId(desc.name, *id);
		}
	}
	return *id;
}

void Hush::Scene::AddEngineSystem(ISystem *system)
{
	m_engineSystems.push_back(system);
	SortSystems();
}

void Hush::Scene::AddScriptingSystem(uintptr_t system)
{
	this->m_scriptingSystems.push_back(system);
	// If the system is added in the middle of a frame, we should always call init
	if (this->m_isInitialized)
	{
		this->m_scriptingInterface->initFunction(reinterpret_cast<void *>(system));
	}
}

void Hush::Scene::SortSystems()
{
	// First, clear the buckets
	for (std::vector<ISystem *> &bucket : m_systems)
	{
		bucket.clear();
	}

	// Then, add the engine systems
	for (ISystem *system : m_engineSystems)
	{
		m_systems[system->Order()].push_back(system);
	}

	// Finally, add the user systems
	for (std::unique_ptr<ISystem> &system : m_userSystems)
	{
		m_systems[system->Order()].push_back(system.get());
	}
}
