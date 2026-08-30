/*! \file Scene.cpp
	\author Alan Ramirez
	\date 2025-01-20
	\brief Scene implementation
*/

#include "Scene.hpp"
#include "Assertions.hpp"
#include "Components/ComponentMetadata.hpp"
#include "Components/Serializable.hpp"
#include "Components/WorldTransform.hpp"
#include "Entity.hpp"
#include "ISystem.hpp"
#include "Logger.hpp"
#include "SceneAsset.hpp"
#include "serialization/Formats/JsonSerializer.hpp"
#include "serialization/Serialization.hpp"
#include "serialization/SerializedEntity.hpp"
#include "utils/ParallelUtils.hpp"
#include <array>
#include <cstdint>
#include <flecs.h>
#include <flecs/addons/flecs_c.h>
#include "Profiling.hpp"
#include <flecs/private/api_defines.h>
#include <magic_enum/magic_enum.hpp>
#include <memory>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

constexpr std::size_t DEFAULT_SYSTEMS_CAPACITY = 128;

std::atomic<std::uint64_t> Hush::Scene::s_nextSceneId{1};

Hush::Scene::Scene(HushEngine *engine, Hush::Threading::Executors::ThreadPool *threadPool)
	: m_engine(engine),
	  m_threadPool(threadPool),
	  m_world(ecs_init()),
	  m_sceneId(s_nextSceneId.fetch_add(1, std::memory_order_relaxed))
{
	// Reserve the buckets
	m_userSystems.reserve(DEFAULT_SYSTEMS_CAPACITY);
}

Hush::Scene::~Scene()
{
	// ecs_fini runs component binding_ctx_free hooks, which destroy any objects allocated from
	// the scene arena. Rewind the arena only afterwards, once nothing references it.
	ecs_fini(static_cast<ecs_world_t *>(m_world));

	if (m_sceneMemory != nullptr)
	{
		m_sceneMemory->Reset();
	}
}

void Hush::Scene::HookEvents()
{
	// Register an observer for our inspectable components
	this->AddComponentObserver<InspectableComponent>(EComponentObserverType::Add,
													 [this](Entity::EntityId compId, InspectableComponent *) {
														 // HACK: Add the serializable component to it
														 this->m_registeredComponents.emplace_back(compId);
													 });
}

void Hush::Scene::Init()
{
	ZoneScoped;
	if (!this->m_isInitialized)
	{
		// Skip built-in systems on re-call

		for (const std::vector<ISystem *> &systemBucket : m_systems)
		{

#if HUSH_PLATFORM_EMSCRIPTEN
			for (ISystem *system : systemBucket)
			{
				system->Init();
			}
#else
			Threading::Wait(Threading::ParallelFor(m_threadPool, systemBucket.begin(), systemBucket.end(),
												   [](ISystem *system) { system->Init(); }));
#endif
		}
	}

	// TODO: Group user systems into buckets
	if (this->m_scriptingInterface != nullptr)
	{
		ScriptingSystemInterface::CallSystemInit_t initFunc = this->m_scriptingInterface->initFunction;
#if HUSH_PLATFORM_EMSCRIPTEN
		for (uintptr_t system : this->m_scriptingSystems)
		{
			initFunc(reinterpret_cast<void *>(system));
		}
#else
		Threading::Wait(
			Threading::ParallelFor(m_threadPool, this->m_scriptingSystems.begin(), this->m_scriptingSystems.end(),
								   [initFunc](uintptr_t system) { initFunc(reinterpret_cast<void *>(system)); }));
#endif
	}
	else
	{
		LogFormat(
			ELogLevel::Warn,
			"FIXME: The scripting interface should be set, having a nullptr is only tolerated for testing purposes!");
	}

	this->m_isInitialized = true;
}

void Hush::Scene::Update(float delta, bool isEditor)
{
	ZoneScoped;
	for (const std::vector<ISystem *> &systemBucket : m_systems)
	{
#if HUSH_PLATFORM_EMSCRIPTEN
		for (ISystem *system : systemBucket)
		{
			system->OnUpdate(delta);
		}
#else
		Threading::Wait(
			Threading::ParallelFor(m_threadPool, systemBucket.begin(), systemBucket.end(), [delta](ISystem *system) {
				// Call the update method for each system
				system->OnUpdate(delta);
			}));
#endif
	}

	// BACKLOG: Make user systems be able to run at editor time
	if (this->m_scriptingInterface == nullptr || isEditor)
	{
		return;
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
	ZoneScoped;

	for (const std::vector<ISystem *> &systemBucket : m_systems)
	{
#if HUSH_PLATFORM_EMSCRIPTEN
		for (ISystem *system : systemBucket)
		{
			system->OnFixedUpdate(delta);
		}
#else
		Threading::Wait(
			Threading::ParallelFor(m_threadPool, systemBucket.begin(), systemBucket.end(), [delta](ISystem *system) {
				// Call the fixed update method for each system
				system->OnFixedUpdate(delta);
			}));
#endif
	}

	if (this->m_scriptingInterface == nullptr)
	{
		// LogFormat(ELogLevel::Warn, "FIXME: The scripting interface should be set, having a nullptr is only tolerated
		// for testing purposes!");
		return;
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

	ZoneScoped;

	for (const std::vector<ISystem *> &systemBucket : m_systems)
	{
#if HUSH_PLATFORM_EMSCRIPTEN
		for (ISystem *system : systemBucket)
		{
			system->OnPreRender();
		}
#else
		Threading::Wait(Threading::ParallelFor(m_threadPool, systemBucket.begin(), systemBucket.end(),
											   [](ISystem *system) { system->OnPreRender(); }));
#endif
	}

	if (this->m_scriptingInterface == nullptr)
	{
		// LogFormat(ELogLevel::Warn, "FIXME: The scripting interface should be set, having a nullptr is only tolerated
		// for testing purposes!");
		return;
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

	ZoneScoped;

	for (const std::vector<ISystem *> &systemBucket : m_systems)
	{
#if HUSH_PLATFORM_EMSCRIPTEN
		for (ISystem *system : systemBucket)
		{
			system->OnRender();
		}
#else
		Threading::Wait(Threading::ParallelFor(m_threadPool, systemBucket.begin(), systemBucket.end(),
											   [](ISystem *system) { system->OnRender(); }));
#endif
	}

	if (this->m_scriptingInterface == nullptr)
	{
		// LogFormat(ELogLevel::Warn, "FIXME: The scripting interface should be set, having a nullptr is only tolerated
		// for testing purposes!");
		return;
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

	ZoneScoped;

	for (const std::vector<ISystem *> &systemBucket : m_systems)
	{
#if HUSH_PLATFORM_EMSCRIPTEN
		for (ISystem *system : systemBucket)
		{
			system->OnPostRender();
		}
#else
		Threading::Wait(Threading::ParallelFor(m_threadPool, systemBucket.begin(), systemBucket.end(),
											   [](ISystem *system) { system->OnPostRender(); }));
#endif
	}

	if (this->m_scriptingInterface == nullptr)
	{
		// LogFormat(ELogLevel::Warn, "FIXME: The scripting interface should be set, having a nullptr is only tolerated
		// for testing purposes!");
		return;
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

	ZoneScoped;

	for (const std::vector<ISystem *> &systemBucket : m_systems)
	{
#if HUSH_PLATFORM_EMSCRIPTEN
		for (ISystem *system : systemBucket)
		{
			system->OnShutdown();
		}
#else
		Threading::Wait(Threading::ParallelFor(m_threadPool, systemBucket.begin(), systemBucket.end(),
											   [](ISystem *system) { system->OnShutdown(); }));
#endif
	}

	if (this->m_scriptingInterface == nullptr)
	{
		// LogFormat(ELogLevel::Warn, "FIXME: The scripting interface should be set, having a nullptr is only tolerated
		// for testing purposes!");
		return;
	}
	ScriptingSystemInterface::CallSystemOnShutdown_t shutdownFunc = this->m_scriptingInterface->shutdownFunction;
	// TODO: Sort in threading
	for (uintptr_t system : this->m_scriptingSystems)
	{
		shutdownFunc(reinterpret_cast<void *>(system));
	}
}

// Free helper functions
inline bool ShouldContinueReadingCompsArray(Hush::Serialization::JsonDeserializer::EToken tk)
{
	using JsonEToken_t = Hush::Serialization::JsonDeserializer::EToken;
	return tk != JsonEToken_t::ArrayEnd && tk != JsonEToken_t::Error && tk != JsonEToken_t::EndOfInput;
}

// Local macro helper
#define BREAK_LOOP_IF_NEEDED                                                                                           \
	currToken = deserializer.GetToken();                                                                               \
	if (!ShouldContinueReadingCompsArray(currToken))                                                                   \
	{                                                                                                                  \
		break;                                                                                                         \
	}

inline Hush::Scene::EError DeserializeComponents(
	Hush::Scene *scene,
	Hush::Serialization::JsonDeserializer &deserializer,
	Hush::Entity &entity,
	std::vector<std::pair<Hush::Entity::EntityId, Hush::Entity::EntityId>> *pendingPairs)
{
	using namespace Hush;
	deserializer.Next(); // Skip the start of the array
	std::string_view currKey;
	int64_t compId{};
	std::string_view compKey{};
	Serialization::JsonDeserializer::EToken currToken{};
	for (currToken = deserializer.GetToken(); ShouldContinueReadingCompsArray(currToken);
		 currToken = deserializer.GetToken())
	{
		std::string_view objectJson;
		deserializer.PeekObject(objectJson);

		Serialization::JsonDeserializer localDeser{objectJson};

		deserializer.Next();
		BREAK_LOOP_IF_NEEDED;

		deserializer.ReadKey(currKey);
		BREAK_LOOP_IF_NEEDED;
		deserializer.ReadInt(compId);
		BREAK_LOOP_IF_NEEDED;

		deserializer.ReadKey(currKey);
		BREAK_LOOP_IF_NEEDED;
		deserializer.ReadString(compKey);
		BREAK_LOOP_IF_NEEDED;

		// Pair IDs have bit 63 set and have zero size — they cannot be added as normal
		// components. Defer them to the post-loop remapping pass.
		auto savedCompId = static_cast<Entity::EntityId>(compId);
		if (pendingPairs != nullptr && Entity::IsPairId(savedCompId))
		{
			pendingPairs->emplace_back(entity.GetId(), savedCompId);
			deserializer.SkipObject();
			BREAK_LOOP_IF_NEEDED;
			continue;
		}

		// Get the component with that key
		Entity comp = scene->CreateEntityWithKey(compKey);
		void *instance = entity.AddComponentRaw(comp.GetId());

		// Deserialize it
		Serializable *serComp = comp.GetComponent<Serializable>();
		if (serComp != nullptr && serComp->deserialize != nullptr)
		{
			auto *rawInstance = reinterpret_cast<uint8_t *>(instance);
			serComp->deserialize(rawInstance, localDeser, serComp->ctx);
			if (serComp->postDeserialize != nullptr)
			{
				serComp->postDeserialize(rawInstance, entity.GetId(), serComp->type, serComp->ctx);
			}
		}

		deserializer.SkipObject();
		BREAK_LOOP_IF_NEEDED;
	}
	return Scene::EError::None;
}

Hush::Scene::EError Hush::Scene::FromSceneAsset(const std::string &asset)
{
	Serialization::JsonDeserializer deserializer{asset};
	// Enter the object
	HUSH_COND_FAIL_V(deserializer.Next(), EError::BadSceneFormat);
	std::string_view currKey{};
	HUSH_COND_FAIL_V(deserializer.ReadKey(currKey), EError::BadSceneFormat);
	// This should be the entities array now
	HUSH_COND_FAIL_V(deserializer.Next(), EError::BadSceneFormat);
	// Maps old session entity index (lower 32 bits of serialized ID) → new runtime entity ID.
	std::unordered_map<Entity::EntityId, Entity::EntityId> idRemap;
	// Relationship pairs deferred until all entities are created.
	std::vector<std::pair<Entity::EntityId, Entity::EntityId>> pendingPairs;

	// We are now on our object
	while (deserializer.Next())
	{
		// Entity structure
		// {"id": ##, "key": "...", "components": [...]}
		int64_t id{};
		deserializer.ReadKey(currKey);
		deserializer.ReadInt(id);
		std::string_view entKey;
		deserializer.ReadKey(currKey);
		deserializer.ReadString(entKey);

		Entity ent;
		if (entKey.empty())
		{
			ent = this->CreateEntity();
		}
		else
		{
			ent = this->CreateEntityWithKey(entKey);
		}
		// Record old entity index → new entity ID for relationship remapping.
		idRemap[static_cast<Entity::EntityId>(id) & 0xFFFFFFFFULL] = ent.GetId();

		// We don't care abt this one
		deserializer.ReadKey(currKey);
		// Then we can go for comps related to that entity
		std::string_view compsArray;
		if (!deserializer.ReadArray(compsArray))
		{
			continue;
		}
		Serialization::JsonDeserializer arrayDeser{compsArray};
		DeserializeComponents(this, arrayDeser, ent, &pendingPairs);
		deserializer.Next();
	}

	// Second pass: reconstruct relationship pairs with remapped entity IDs.
	// Built-in flecs entities (e.g. EcsChildOf) are not in idRemap; fall back to their original ID.
	auto *flecsWorld = static_cast<ecs_world_t *>(m_world);
	for (const auto &[entityId, savedPairId] : pendingPairs)
	{
		Entity::EntityId oldFirst  = Entity::GetPairFirst(savedPairId);
		Entity::EntityId oldSecond = Entity::GetPairSecond(savedPairId);
		Entity::EntityId newFirst  = idRemap.count(oldFirst)  ? idRemap.at(oldFirst)  : oldFirst;
		Entity::EntityId newSecond = idRemap.count(oldSecond) ? idRemap.at(oldSecond) : oldSecond;
		ecs_add_id(flecsWorld, entityId, ecs_pair(newFirst, newSecond));
	}

	(void)asset;
	return EError::None;
}

#undef BREAK_LOOP_IF_NEEDED

Hush::Scene::EError Hush::Scene::ToSceneAsset(std::string &asset)
{
	// HUSH_ASSERT(asset != nullptr, "Cannot serialize to an invalid scene asset handle");
	// Serialize every single entity in the world with each of its components
	// TODO: For now, every entity that has a transform is enough, but there are
	// use cases where we want to serialize raw entities with no inspectable transoforms
	auto q = this->CreateQuery<WorldTransform>();
	auto *world = static_cast<ecs_world_t *>(m_world);
	EntityId serializableId = this->RegisterComponent<Serializable>();

	Serialization::JsonSerializer jsonSerializer{};
	Serialization::ESerializationError serialErr{};
	serialErr = jsonSerializer.BeginObject();
	serialErr = jsonSerializer.SetKey("entities");
	serialErr = jsonSerializer.BeginArray();
	q.Each([world, serializableId, scene = this, &jsonSerializer, &serialErr](Entity &ent, WorldTransform &xform) {
		(void)xform;
		serialErr = jsonSerializer.BeginObject();
		serialErr = jsonSerializer.Serialize("id", ent.GetId());
		std::string_view topKey = ent.GetKey();
		// If there's no key, we save the ID as the key
		serialErr = jsonSerializer.Serialize("key", topKey);

		serialErr = jsonSerializer.SetKey("components");
		serialErr = jsonSerializer.BeginArray();
		ent.EachId([world, serializableId, scene, &ent, &jsonSerializer, &serialErr](Entity::EntityId comp) {
			// Pairs have size 0; ecs_get_id would assert. Serialize just the id so
			// deserialization can detect them via IsPairId and remap old→new entity indices.
			// Cast to int64_t so rapidjson writes a signed int that ReadInt can parse back
			// (the pair ID has bit 63 set and would exceed INT64_MAX as unsigned).
			if (Entity::IsPairId(comp))
			{
				constexpr const char* emptyStr = "";
				serialErr = jsonSerializer.BeginObject();
				serialErr = jsonSerializer.Serialize("id", static_cast<int64_t>(comp));
				serialErr = jsonSerializer.Serialize("key", std::string_view{emptyStr});
				serialErr = jsonSerializer.EndObject();
				return;
			}
			serialErr = jsonSerializer.BeginObject();
			const auto *rawComp = reinterpret_cast<const uint8_t *>(ecs_get_id(world, ent.GetId(), comp));
			// Serialize comp to JSON
			const char *key = ecs_get_name(world, comp);
			// Not likely to be nullptr, but we do it anyways
			serialErr = jsonSerializer.Serialize("id", comp);
			serialErr = jsonSerializer.Serialize("key", key == nullptr ? std::string_view{} : std::string_view(key));
			// Find the serialization comp
			// NOLINTNEXTLINE
			const auto *serializer = reinterpret_cast<const Serializable *>(ecs_get_id(world, comp, serializableId));

			if (serializer == nullptr)
			{
				// This is okay we just push the fact that this component exists
				serialErr = jsonSerializer.EndObject();
				return;
			}

			Serializable::EError err = serializer->serialize(rawComp, jsonSerializer, serializer->ctx);

			serialErr = jsonSerializer.EndObject();
			if (err != Serializable::EError::None)
			{
				LogFormat(ELogLevel::Error, "Failed to serialize component {}, error: {}. Skipping!", key,
						  magic_enum::enum_name(err));
				return;
			}
		});
		serialErr = jsonSerializer.EndArray();
		serialErr = jsonSerializer.EndObject();
	});
	serialErr = jsonSerializer.EndArray();
	serialErr = jsonSerializer.EndObject();

	(void)serialErr;

	asset.assign(jsonSerializer.FinishSerialization());

	return EError::None;
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

Hush::Entity Hush::Scene::CreateEntityWithKey(std::string_view key)
{
	auto *world = static_cast<ecs_world_t *>(m_world);
	ecs_entity_desc_t desc = {.name = key.data()};
	const Entity::EntityId entityId = ecs_entity_init(world, &desc);
	Entity result{this, entityId};
	return result;
}

void Hush::Scene::AddComponentObserverRaw(Entity::EntityId componentId, size_t componentSize,
										  EComponentObserverType observerType, ObserverCallback_t callback)
{
	HUSH_ASSERT(callback != nullptr, "Cannot add a component observer with a null callback!");
	Entity::EntityId event = this->ObserverTypeToEntityId(observerType);

	auto *world = static_cast<ecs_world_t *>(this->m_world);
	ecs_term_t queryTerm = {.id = componentId, .inout = EcsIn};
	ecs_query_desc_t query = {.terms = {queryTerm}};

	// TODO: Replace heap for arena allocator
	struct CallbackContext
	{
		ObserverCallback_t function;
		size_t componentByteSize;
	};

	auto *context = new CallbackContext();
	context->function = callback;
	context->componentByteSize = componentSize;

	ecs_observer_desc_t observerDesc = {
		.query = query,
		.events = {event},
		.callback =
			[](ecs_iter_t *it) {
				if (it->count <= 0)
				{
					return;
				}
				Entity::EntityId eventEntity = it->entities[0];

				auto *callbackCtx = reinterpret_cast<CallbackContext *>(it->callback_ctx);
				void *componentInstance = ecs_field_w_size(it, callbackCtx->componentByteSize, 0);

				callbackCtx->function(eventEntity, componentInstance);
				// BUG: Memory leak, the outer function must return the observer's Id and that indicates ownershipp
			},
		.callback_ctx = context,
		.callback_ctx_free = [](void *ctx) { delete static_cast<CallbackContext *>(ctx); }};

	[[maybe_unused]]
	Entity::EntityId observerId = ecs_observer_init(world, &observerDesc);
}

Hush::Entity::EntityId Hush::Scene::AddEventObserverRaw(Entity::EntityId event, EntityEventCallback_t callback)
{
	auto *world = static_cast<ecs_world_t *>(this->m_world);

	ecs_term_t queryTerm = {.id = EcsAny};
	ecs_query_desc_t query = {.terms = {queryTerm}};

	// TODO: Replace heap for arena allocator
	ecs_observer_desc_t observerDesc = {.query = query,
										.events = {event},
										.callback =
											[](ecs_iter_t *it) {
												if (it->count <= 0)
												{
													return;
												}
												Entity::EntityId eventEntity = it->entities[0];

												auto callback =
													reinterpret_cast<EntityEventCallback_t>(it->callback_ctx);
												auto *scene = reinterpret_cast<Scene *>(it->run_ctx);
												callback(eventEntity, scene);
											},
										.callback_ctx = reinterpret_cast<void *>(callback),
										.run_ctx = reinterpret_cast<void *>(this)};

	// So the observer can be removed
	return ecs_observer_init(world, &observerDesc);
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

std::optional<std::uint64_t> Hush::Scene::GetRegisteredComponentId(NullTerminatedStringView name)
{
	std::shared_lock lock(m_registeredEntitiesMutex);
	const auto entityIt = m_registeredEntities.find(std::string(std::string_view(name)));

	if (entityIt != m_registeredEntities.end())
	{
		return entityIt->second;
	}

	// Component not found.
	return std::nullopt;
}

void Hush::Scene::RegisterComponentId(NullTerminatedStringView name, Entity::EntityId id)
{
	std::unique_lock lock(m_registeredEntitiesMutex);
	m_registeredEntities.insert_or_assign(std::string(std::string_view(name)), id);
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

		// The resource this instance was allocated from, so binding_ctx_free can return the
		// storage symmetrically (a no-op for the scene arena, an actual free for the fallback).
		std::pmr::memory_resource *ownerResource{};
	};

	// This context lives for the scene/world lifetime, so it belongs in the scene arena. When
	// the arena is not wired yet, fall back to the general heap; either way the free hook below
	// routes deallocation back through ownerResource.
	std::pmr::memory_resource *ownerResource = m_sceneMemory != nullptr
												   ? static_cast<std::pmr::memory_resource *>(m_sceneMemory)
												   : std::pmr::new_delete_resource();

	void *storage = ownerResource->allocate(sizeof(ComponentInfo), alignof(ComponentInfo));
	auto *componentInfo = std::construct_at(static_cast<ComponentInfo *>(storage));
	componentInfo->size = desc.size;
	componentInfo->alignment = desc.alignment;
	componentInfo->name = desc.name;
	componentInfo->ops = desc.ops;
	componentInfo->userCtx = desc.userCtx;
	componentInfo->userCtxFree = desc.userCtxFree;
	componentInfo->ownerResource = ownerResource;
	componentInfo->opsFlags = desc.opsFlags;

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
		auto *info = static_cast<ComponentInfo *>(ctx);
		if (info->userCtxFree != nullptr)
		{
			info->userCtxFree(info->userCtx);
		}
		std::pmr::memory_resource *ownerResource = info->ownerResource;
		std::destroy_at(info);
		ownerResource->deallocate(info, sizeof(ComponentInfo), alignof(ComponentInfo));
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

	LogFormat(ELogLevel::Info, "Registered component with name {} as ID: {}", desc.name, componentId);

	return componentId;
}

void Hush::Scene::MarkComponentToggleableRaw(EntityId id)
{
	auto *world = static_cast<ecs_world_t *>(this->m_world);
	ecs_add_id(world, id, EcsCanToggle);
}

Hush::Entity::EntityId Hush::Scene::Lookup(NullTerminatedStringView tag) const
{
	auto *world = static_cast<ecs_world_t *>(this->m_world);
	return ecs_lookup(world, tag.c_str());
}

Hush::Entity::EntityId Hush::Scene::Lookup(std::string_view tag) const
{
	auto *world = static_cast<ecs_world_t *>(this->m_world);
	return ecs_lookup(world, tag.data());
}

const std::vector<Hush::Entity::EntityId> &Hush::Scene::GetAllRegisteredComponents() const
{
	return this->m_registeredComponents;
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
		// desc.name is a null-terminated C string (from GetTypeName<T>()), but a plain const
		// char* does not implicitly convert to NullTerminatedStringView, so wrap it explicitly.
		const NullTerminatedStringView componentName =
			NullTerminatedStringView::promise_null_terminated(std::string_view{desc.name});

		// First, check if the component is already registered in the scene.
		if (auto cachedComponentId = GetRegisteredComponentId(componentName); cachedComponentId.has_value())
		{
			// Okay, already registered in the scene by another thread or translation unit.
			*id = *cachedComponentId;
		}
		else
		{
			// We need to register the component.
			*id = RegisterComponentRaw(desc);
			RegisterComponentId(componentName, *id);
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
