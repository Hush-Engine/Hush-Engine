/*! \file Scene.cpp
    \author Alan Ramirez
    \date 2025-01-20
    \brief Scene implementation
*/

#include "Scene.hpp"

#define FLECS_NO_CPP
#include <flecs.h>

constexpr std::size_t DEFAULT_SYSTEMS_CAPACITY = 128;

Hush::Scene::Scene(HushEngine *engine)
    : m_engine(engine),
      m_world(ecs_init())
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
    // Init all systems
    for (const std::vector<ISystem *> &systemBucket : m_systems)
    {
        for (ISystem *system : systemBucket)
        {
            system->Init();
        }
    }
}

void Hush::Scene::Update(float delta)
{
    for (const std::vector<ISystem *> &systemBucket : m_systems)
    {
        for (ISystem *system : systemBucket)
        {
            system->OnUpdate(delta);
        }
    }
}

void Hush::Scene::FixedUpdate(float delta)
{
    for (const std::vector<ISystem *> &systemBucket : m_systems)
    {
        for (ISystem *system : systemBucket)
        {
            system->OnFixedUpdate(delta);
        }
    }
}

void Hush::Scene::PreRender()
{
    for (const std::vector<ISystem *> &systemBucket : m_systems)
    {
        for (ISystem *system : systemBucket)
        {
            system->OnPreRender();
        }
    }
}
void Hush::Scene::Render()
{
    for (const std::vector<ISystem *> &systemBucket : m_systems)
    {
        for (ISystem *system : systemBucket)
        {
            system->OnRender();
        }
    }
}

void Hush::Scene::PostRender()
{
    for (const std::vector<ISystem *> &systemBucket : m_systems)
    {
        for (ISystem *system : systemBucket)
        {
            system->OnPostRender();
        }
    }
}

void Hush::Scene::Shutdown()
{
    for (const std::vector<ISystem *> &systemBucket : m_systems)
    {
        for (ISystem *system : systemBucket)
        {
            system->OnShutdown();
        }
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

Hush::Entity Hush::Scene::CreateEntityWihName(std::string_view name)
{
    auto *world = static_cast<ecs_world_t *>(m_world);

    const ecs_entity_desc_t desc = {
        .name = name.data(),
    };

    const Entity::EntityId entityId = ecs_entity_init(world, &desc);

    return Entity{this, entityId};
}

void Hush::Scene::DestroyEntity(Entity &&entity)
{
    auto entityToDestroy = std::move(entity);
    auto *world = static_cast<ecs_world_t *>(m_world);

    ecs_delete(world, entityToDestroy.GetId());
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

Hush::Entity::EntityId Hush::Scene::RegisterComponentRaw(const ComponentTraits::ComponentInfo &desc) const
{
    struct ComponentInfo
    {
        std::size_t size{};
        std::size_t alignment{};
        std::string name;
        ComponentTraits::ComponentOps ops{};

        void *userCtx{};
        void (*userCtxFree)(void *){};
    };

    auto *componentInfo = new ComponentInfo();
    componentInfo->size = desc.size;
    componentInfo->alignment = desc.alignment;
    componentInfo->name = desc.name;
    componentInfo->ops = desc.ops;
    componentInfo->userCtx = desc.userCtx;
    componentInfo->userCtxFree = desc.userCtxFree;

    ecs_component_desc_t componentDesc = {};
    componentDesc.type.alignment = static_cast<ecs_size_t>(componentInfo->alignment);
    componentDesc.type.size = static_cast<ecs_size_t>(componentInfo->size);
    componentDesc.type.name = componentInfo->name.data();
    componentDesc.type.hooks.binding_ctx = componentInfo;

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
                .name = info->name,
                .ops = info->ops,
                .userCtx = info->userCtx,
                .userCtxFree = info->userCtxFree,
            };

            info->ops.ctor(ptr, count, componentDesc);
        };
    }

    if (desc.ops.dtor != nullptr)
    {
        componentDesc.type.hooks.dtor = [](void *ptr, int32_t count, const ecs_type_info_t *type_info) {
            const auto *info = static_cast<ComponentInfo *>(type_info->hooks.binding_ctx);

            ComponentTraits::ComponentInfo componentDesc = {
                .size = info->size,
                .alignment = info->alignment,
                .name = info->name,
                .ops = info->ops,
                .userCtx = info->userCtx,
                .userCtxFree = info->userCtxFree,
            };

            info->ops.dtor(ptr, count, componentDesc);
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
                .name = info->name,
                .ops = info->ops,
                .userCtx = info->userCtx,
                .userCtxFree = info->userCtxFree,
            };

            info->ops.copy(dst, src, count, componentDesc);
        };
    }

    if (desc.ops.move != nullptr)
    {
        componentDesc.type.hooks.move = [](void *dst, void *src, int32_t count, const ecs_type_info_t *type_info) {
            const auto *info = static_cast<ComponentInfo *>(type_info->hooks.binding_ctx);

            ComponentTraits::ComponentInfo componentDesc = {
                .size = info->size,
                .alignment = info->alignment,
                .name = info->name,
                .ops = info->ops,
                .userCtx = info->userCtx,
                .userCtxFree = info->userCtxFree,
            };

            info->ops.move(dst, src, count, componentDesc);
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
                .name = info->name,
                .ops = info->ops,
                .userCtx = info->userCtx,
                .userCtxFree = info->userCtxFree,
            };

            info->ops.copyCtor(dst, src, count, componentDesc);
        };
    }

    if (desc.ops.moveCtor != nullptr)
    {
        componentDesc.type.hooks.move_ctor = [](void *dst, void *src, int32_t count, const ecs_type_info_t *type_info) {
            const auto *info = static_cast<ComponentInfo *>(type_info->hooks.binding_ctx);

            ComponentTraits::ComponentInfo componentDesc = {
                .size = info->size,
                .alignment = info->alignment,
                .name = info->name,
                .ops = info->ops,
                .userCtx = info->userCtx,
                .userCtxFree = info->userCtxFree,
            };

            info->ops.moveCtor(dst, src, count, componentDesc);
        };
    }

    if (desc.ops.moveDtor != nullptr)
    {
        componentDesc.type.hooks.move_dtor = [](void *dst, void *src, int32_t count, const ecs_type_info_t *type_info) {
            const auto *info = static_cast<ComponentInfo *>(type_info->hooks.binding_ctx);

            ComponentTraits::ComponentInfo componentDesc = {
                .size = info->size,
                .alignment = info->alignment,
                .name = info->name,
                .ops = info->ops,
                .userCtx = info->userCtx,
                .userCtxFree = info->userCtxFree,
            };

            info->ops.moveDtor(dst, src, count, componentDesc);
        };
    }

    if (desc.ops.moveAssignDtor != nullptr)
    {
        componentDesc.type.hooks.move_dtor = [](void *dst, void *src, int32_t count, const ecs_type_info_t *type_info) {
            const auto *info = static_cast<ComponentInfo *>(type_info->hooks.binding_ctx);

            ComponentTraits::ComponentInfo componentDesc = {
                .size = info->size,
                .alignment = info->alignment,
                .name = info->name,
                .ops = info->ops,
                .userCtx = info->userCtx,
                .userCtxFree = info->userCtxFree,
            };

            info->ops.moveAssignDtor(dst, src, count, componentDesc);
        };
    }

    componentDesc.type.hooks.flags = static_cast<std::uint32_t>(desc.opsFlags);

    // Register the component
    auto *world = static_cast<ecs_world_t *>(GetWorld());
    ecs_entity_t componentId = ecs_component_init(world, &componentDesc);

    return componentId;
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
        }
    }
    return *id;
}

void Hush::Scene::AddEngineSystem(ISystem *system)
{
    m_engineSystems.push_back(system);
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