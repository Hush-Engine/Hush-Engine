/*! \file Scene.hpp
	\author Alan Ramirez
	\date 2025-01-20
	\brief Scene implementation
*/

#pragma once

#include "Assertions.hpp"
#include "Components/Serializable.hpp"
#include "Entity.hpp"
#include "ISystem.hpp"
#include "Logger.hpp"
#include "NullTerminatedStringView.hpp"
#include "Query.hpp"
#include "HushBindings.hpp"
#include "QueryBuilder.hpp"
#include "SceneAsset.hpp"
#include "executors/ThreadPool.hpp"
#include "Hush/Memory/ThreadLocalMemoryResourcePool.hpp"
#include <array>
#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <memory_resource>
#include <shared_mutex>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

#define FLECS_NO_CPP
// We need to add these in for the templated functions, sorry :P
#include <flecs/addons/flecs_c.h>

#include <flecs.h>

namespace Hush
{
	class SceneAsset;
	enum class [[hush::export]] EComponentObserverType
	{
		Add,
		Remove,
		Set
	};

	using ObserverCallback_t = void (*)(Entity::EntityId, void *);

	class HushEngine;

	// TODO: this class is expected to change a lot, it's just a placeholder for now.
	// The API is not ready and I would like to think about implementing it considering scripting in the future and
	// bindings.
	class [[hush::export(Hush::Export::asHandle)]] Scene
	{
		constexpr static std::uint16_t ORDER_BUCKET_SIZE = ISystem::MAX_ORDER + 1;

		friend class HushEngine;

		friend class Entity;

		using EntityId = Entity::EntityId;

	public:
		enum class EError
		{
			None = 0,
			BadSceneFormat
		};

		/// Constructor.
		/// @param engine Game engine
		Scene(HushEngine *engine, Hush::Threading::Executors::ThreadPool *threadPool);

		~Scene();

		/// Init is called when the scene is initialized.
		void Init();

		/// Update is called every frame.
		void Update(float delta);

		/// FixedUpdate is called every fixed frame.
		void FixedUpdate(float delta);

		/// PreRender is called before rendering.
		void PreRender();

		/// Render is called when the scene should render.
		void Render();

		/// PostRender is called after rendering.
		void PostRender();

		/// Shutdown is called when the scene is shutting down.
		void Shutdown();

		///
		/// @tparam S Add a system to the scene
		template <typename S>
			requires std::derived_from<S, ISystem>
		void AddSystem()
		{
			m_userSystems.push_back(std::make_unique<S>());
		}

		/// @brief Parses a scene asset and instantiates all entities and systems in it to this scene
		EError FromSceneAsset(SceneAsset* asset); // TODO: This should be a Ref<SceneAsset>, but the resources module is one layer above us

		EError ToSceneAsset(SceneAsset* asset);

		/// Remove a system from the scene by name.
		/// @param name Name of the system to remove.
		[[hush::export]]
		void RemoveSystem(std::string_view name);

		/// Creates an entity
		[[hush::export]]
		Entity CreateEntity();

		/// Creates an entity with a name
		/// @param name Display name of the entity
		/// @return Entity
		[[nodiscard]] [[hush::export]]
		Entity CreateEntityWithName(std::string_view name);

		[[nodiscard]] [[hush::export]]
		Entity CreateEntityWithKey(std::string_view key);

		/// @brief Registers a callback that gets called whenever a component receives the specified event
		/// @param componentId Component to query for
		/// @param componentSize Size in bytes of the component, needed for ensuring correct data access on your
		/// callback
		/// @param observerType Component event type
		[[hush::export]]
		void AddComponentObserverRaw(Entity::EntityId componentId, size_t componentSize,
									 EComponentObserverType observerType, ObserverCallback_t callback);

		/// Registers a callback that gets called whenever a component receives the specified event
		// @param observerType Component event type
		template <class T, class Func>
			requires std::invocable<Func, Entity::EntityId, T *>
		void AddComponentObserver(EComponentObserverType observerType, Func &&callback)
		{
			Entity::EntityId event = ObserverTypeToEntityId(observerType);
			const Entity::EntityId componentId = RegisterIfNeededSlow<T>();

			auto *world = static_cast<ecs_world_t *>(this->m_world);
			ecs_term_t queryTerm = {.id = componentId, .inout = EcsIn};
			ecs_query_desc_t query = {.terms = {queryTerm}};

			using CallbackFunc_t = std::function<void(Entity::EntityId, T *)>;
			// Horrible hack, but we wanted to use the C API
			// NOLINTNEXTLINE
			auto *function = new CallbackFunc_t(std::forward<Func>(callback));

			ecs_observer_desc_t observerDesc = {.query = query,
												.events = {event},
												.callback =
													[](ecs_iter_t *it) {
														if (it->count <= 0)
														{
															return;
														}
														Entity::EntityId eventEntity = it->entities[0];
														T *component = ecs_field(it, T, 0);
														auto *callbackFunc =
															reinterpret_cast<CallbackFunc_t *>(it->callback_ctx);
														(*callbackFunc)(eventEntity, component);
													},
												.callback_ctx = function,
												.callback_ctx_free =
													[](void *ctx) {
														// NOLINTNEXTLINE
														delete reinterpret_cast<CallbackFunc_t *>(ctx);
													}};
			// TODO: Add this to a member vector so that we can delete it afterwards
			[[maybe_unused]]
			Entity::EntityId observerId = ecs_observer_init(world, &observerDesc);
		}

		/// Destroy an entity.
		/// @param entity Entity to destroy
		void DestroyEntity(Entity &&entity);

		[[hush::export]]
		void DestroyEntity(Entity &entity);

		/// Get the component registered id by name
		/// @param name Component name
		/// @return The component id if it exists, std::nullopt otherwise
		std::optional<std::uint64_t> GetRegisteredComponentId(NullTerminatedStringView name);

		/// Register a component id
		/// @param name Name of the component
		/// @param id Id of the component
		// NOTE: not [[hush::export]] — the binding generator can't yet marshal
		// NullTerminatedStringView (Hush-Engine/hush-llvm#8). Re-export once supported.
		void RegisterComponentId(NullTerminatedStringView name, Entity::EntityId id);

		std::optional<Entity> EntityFromId(EntityId id);

		[[hush::export]]
		Entity EntityFromIdUnchecked(EntityId id);

		[[nodiscard]] [[hush::export]]
		EntityId RegisterComponentRaw(const ComponentTraits::ComponentInfo &desc) const;

		[[hush::export]]
		void MarkComponentToggleableRaw(EntityId id);

		// NOTE: not [[hush::export]] — the binding generator can't yet marshal
		// NullTerminatedStringView (Hush-Engine/hush-llvm#8). Re-export once supported.
		[[nodiscard]]
		EntityId Lookup(NullTerminatedStringView tag) const;

		[[nodiscard]]
		const std::vector<Entity::EntityId> &GetAllRegisteredComponents() const;

		template <typename... Components>
		Query<Components...> CreateQuery(RawQuery::ECacheMode cacheMode = RawQuery::ECacheMode::Default)
		{
			std::array<Entity::EntityId, sizeof...(Components)> components = {RegisterIfNeededSlow<Components>()...};

			auto rawQuery = CreateRawQuery(components, cacheMode);

			return Query<Components...>(std::move(rawQuery));
		}

		template <typename... Components>
		QueryBuilder<Components...> CreateQueryBuilder(RawQuery::ECacheMode cacheMode = RawQuery::ECacheMode::Default)
		{
			(void)cacheMode;
			std::array<Entity::EntityId, sizeof...(Components)> components = {RegisterIfNeededSlow<Components>()...};

			return QueryBuilder<Components...>(this, this->m_world, components);
		}

		/// Add an engine system to the scene
		/// @param system System to add
		void AddEngineSystem(ISystem *system);

		void AddScriptingSystem(uintptr_t system);

		[[hush::export]]
		RawQuery CreateRawQuery(std::span<Entity::EntityId> components,
								RawQuery::ECacheMode cacheMode = RawQuery::ECacheMode::Default);

		/// @brief returns a map of all registered entities without a query (mostly for internal use)
		const std::unordered_map<std::string, Entity::EntityId> &GetAllEntities()
		{
			return this->m_registeredEntities;
		}

		[[nodiscard]]
		HushEngine *GetEngine()
		{
			return this->m_engine;
		}

		[[nodiscard]]
		const HushEngine *GetEngine() const
		{
			return this->m_engine;
		}

		/// Sets the engine-owned frame and scene-scoped memory resources into this scene.
		/// Both may be null, in which case the scene falls back to owning heap allocations.
		void SetMemoryResources(std::pmr::memory_resource *frameMemory,
								Hush::Memory::ThreadLocalMemoryResourcePool *sceneMemory) noexcept
		{
			this->m_frameMemory = frameMemory;
			this->m_sceneMemory = sceneMemory;
		}

		/// The engine-owned frame-scoped memory resource wired into this scene, or null if not
		/// wired. Handy for callers that only hold a `Scene*` and need to materialize a
		/// `NullTerminatedStringView` from a bare view before calling `Lookup` and friends, e.g.
		/// `scene->Lookup(MakeNullTerminated(tag, scene->GetFrameScopeMemoryResource()))`.
		[[nodiscard]]
		std::pmr::memory_resource *GetFrameScopeMemoryResource() const noexcept
		{
			return this->m_frameMemory;
		}

		[[nodiscard]]
		const void *GetWorld() const
		{
			return m_world;
		}

		[[nodiscard]]
		void *GetWorld()
		{
			return m_world;
		}

		/// Globally unique, monotonically increasing identifier assigned at construction.
		[[nodiscard]]
		std::uint64_t GetUniqueId() const noexcept
		{
			return m_sceneId;
		}

		void SetScriptingInterface(ScriptingSystemInterface *scriptingInterface)
		{
			HUSH_ASSERT(scriptingInterface != nullptr, "Scripting interface cannot be null!");
			// TODO: Assert every function here
			this->m_scriptingInterface = scriptingInterface;
		}

		// Public interface for registering with templates, we might do more than registerIfNeededSlow later
		template <class T>
		[[nodiscard]]
		EntityId RegisterComponent()
		{
			return this->RegisterIfNeededSlow<T>();
		}

		template <class T>
		void RegisterDefaultSerializer() {
			Entity comp = this->EntityFromIdUnchecked(this->RegisterComponent<T>());
			Serializable& ser = comp.AddComponent<Serializable>();
			ser.serialize = &Serializable::DefaultSerialize<T>;
		}

	private:
		friend class Entity;
		friend class RawQuery;
		friend class impl::QueryImpl;

		template <typename T>
		[[nodiscard]]
		inline EntityId RegisterIfNeededSlow()
		{
			// First, get the entity id, and check if the component is registered.
			auto [status, componentId] = ComponentTraits::detail::GetEntityId<T>(GetUniqueId());
			const ComponentTraits::ComponentInfo info = ComponentTraits::GetComponentInfo<T>();

			return InternalRegisterCppComponent(status, componentId, info);
		}

		inline Entity::EntityId ObserverTypeToEntityId(EComponentObserverType observerType)
		{
			switch (observerType)
			{
			case EComponentObserverType::Add:
				return EcsOnAdd;
			case EComponentObserverType::Remove:
				return EcsOnRemove;
			case EComponentObserverType::Set:
				return EcsOnSet;
			default:
				// TODO: Error here
				LogFormat(ELogLevel::Error, "Component observer {} not recognized!",
						  static_cast<int32_t>(observerType));
				return Entity::INVALID_ENTITY_ID;
			}
		}

		Entity::EntityId InternalRegisterCppComponent(ComponentTraits::detail::EEntityRegisterStatus registerStatus,
													  std::uint64_t *id, const ComponentTraits::ComponentInfo &desc);

		/// Sort the systems based on their order and store them in the buckets
		void SortSystems();

		/// Ordered array of systems
		/// This is not the most efficient way to store the systems btw.
		std::array<std::vector<ISystem *>, ORDER_BUCKET_SIZE> m_systems;

		/// Map of registered entities
		std::unordered_map<std::string, Entity::EntityId> m_registeredEntities;

		/// Mutex to protect the registered entities
		std::shared_mutex m_registeredEntitiesMutex;

		/// Special vector to store the systems that come from the engine.
		std::vector<ISystem *> m_engineSystems;

		/// User systems (mostly to use with C++-side gameplay code)
		std::vector<std::unique_ptr<ISystem>> m_userSystems;

		/// User systems handled by the scripting host
		std::vector<uintptr_t> m_scriptingSystems;

		// Small ref array of all registered components, used for editor and scripting
		// Components will not be unloaded by the scene until it ends, so this works fine as a real-time cache
		// If this changes, we need to use a flecs query to fetch all entities with the EcsComponent tag
		mutable std::vector<Entity::EntityId> m_registeredComponents;

		HushEngine *m_engine;

		/// Frame-scoped memory resource (owned by the engine): transient allocations such as
		/// null-terminated string copies at C-API boundaries. Null until wired by the engine.
		std::pmr::memory_resource *m_frameMemory = nullptr;

		/// Scene-scoped bump allocator (owned by the engine). Allocations live until the scene
		/// is torn down, at which point ~Scene rewinds it. Null until wired by the engine.
		Hush::Memory::ThreadLocalMemoryResourcePool *m_sceneMemory = nullptr;

		/// Thread pool used by the scene for parallel operations
		Threading::Executors::ThreadPool *m_threadPool;

		void *m_world;

		ScriptingSystemInterface *m_scriptingInterface = nullptr;

		bool m_isInitialized = false;

		/// Globally unique, monotonically increasing identifier. Used as a stable cache key
		/// in ComponentTraits::detail::GetEntityIdImpl. Pointer values can be recycled by the
		/// allocator after a Scene is destroyed; this counter cannot.
		std::uint64_t m_sceneId;

		static std::atomic<std::uint64_t> s_nextSceneId;
	};
} // namespace Hush
