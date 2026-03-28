/*! \file Scene.hpp
	\author Alan Ramirez
	\date 2025-01-20
	\brief Scene implementation
*/

#pragma once

#include "Entity.hpp"
#include "ISystem.hpp"
#include "Logger.hpp"
#include "Query.hpp"
#include "HushBindings.hpp"
#include "QueryBuilder.hpp"
#include "executors/ThreadPool.hpp"

#include <array>
#include <cstdint>
#include <functional>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#define FLECS_NO_CPP
// We need to add these in for the templated functions, sorry :P
#include <flecs/addons/flecs_c.h>

#include <flecs.h>

namespace Hush
{
	enum class EComponentObserverType : int32_t
	{
		Add,
		Remove,
		Set
	};
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

		/// Remove a system from the scene by name.
		/// @param name Name of the system to remove.
		[[hush::export]]
		void RemoveSystem(std::string_view name);

		/// Creates an entity
		[[hush::export]]
		Entity CreateEntity();

		/// Creates an entity with a name
		/// @param name Unique name of the entity
		/// @return Entity
		[[hush::export]]
		Entity CreateEntityWithName(std::string_view name);

		/// Registers a callback that gets called whenever a component receives the specified event
		// @param observerType Component event type
		template <class T, class Func>
		void AddComponentObserver(EComponentObserverType observerType, Func &&callback)
		{
			Entity::EntityId event = 0;
			switch (observerType)
			{
			case EComponentObserverType::Add:
				event = EcsOnAdd;
				break;
			case EComponentObserverType::Remove:
				event = EcsOnRemove;
				break;
			case EComponentObserverType::Set:
				event = EcsOnSet;
				break;
			default:
				// TODO: Error here
				LogFormat(ELogLevel::Error, "Component observer {} not recognized!",
						  static_cast<int32_t>(observerType));
				return;
			}

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
			Entity::EntityId observerId = ecs_observer_init(world, &observerDesc);
		}

		/// Destroy an entity.
		/// @param entity Entity to destroy
		void DestroyEntity(Entity &&entity);

		void DestroyEntity(Entity &entity);

		/// Get the component registered id by name
		/// @param name Component name
		/// @return The component id if it exists, std::nullopt otherwise
		std::optional<std::uint64_t> GetRegisteredComponentId(std::string_view name);

		/// Register a component id
		/// @param name Name of the component
		/// @param id Id of the component
		[[hush::export]]
		void RegisterComponentId(std::string_view name, Entity::EntityId id);

		std::optional<Entity> EntityFromId(EntityId id);

		[[hush::export]]
		Entity EntityFromIdUnchecked(EntityId id);

		[[nodiscard]] [[hush::export]]
		EntityId RegisterComponentRaw(const ComponentTraits::ComponentInfo &desc) const;

		[[nodiscard]] [[hush::export]]
		EntityId Lookup(std::string_view tag) const;

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

		[[hush::export]]
		RawQuery CreateRawQuery(std::span<Entity::EntityId> components,
								RawQuery::ECacheMode cacheMode = RawQuery::ECacheMode::Default);

		/// @brief returns a map of all registered entities without a query (mostly for internal use)
		const std::unordered_map<std::string, Entity::EntityId> &GetAllEntities()
		{
			return this->m_registeredEntities;
		}

		HushEngine* GetEngine() {
			return this->m_engine;
		}

		[[nodiscard]]
		void *GetWorld() const
		{
			return m_world;
		}
	private:
		friend class Entity;
		friend class RawQuery;
		friend class impl::QueryImpl;

		template <typename T>
		[[nodiscard]]
		EntityId RegisterIfNeededSlow()
		{
			// First, get the entity id, and check if the component is registered.
			auto [status, componentId] = ComponentTraits::detail::GetEntityId<T>(this);
			const ComponentTraits::ComponentInfo info = ComponentTraits::GetComponentInfo<T>();

			return InternalRegisterCppComponent(status, componentId, info);
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

		HushEngine *m_engine;

		/// Thread pool used by the scene for parallel operations
		Threading::Executors::ThreadPool *m_threadPool;

		void *m_world;
	};
} // namespace Hush
