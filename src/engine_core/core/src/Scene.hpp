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

#include <array>
#include <cstdint>
#include <flecs/addons/cpp/world.hpp>
#include <memory>
#include <shared_mutex>
#include <string>
#include <unordered_map>
#include <vector>

// We need to add these in for the templated functions, sorry :P
#include <flecs/addons/flecs_c.h>

// #define FLECS_NO_CPP
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
		Scene(HushEngine *engine);

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
		void AddComponentObserver(EComponentObserverType observerType, Func &&callback) {
			flecs::entity_t event = 0;
			switch (observerType) {
			case EComponentObserverType::Add:
				event = flecs::OnAdd;
				break;
			case EComponentObserverType::Remove:
				event = flecs::OnRemove;
				break;
			case EComponentObserverType::Set:
				event = flecs::OnSet;
				break;
			default:
				// TODO: Error here
				LogFormat(ELogLevel::Error, "Component observer {} not recognized!", static_cast<int32_t>(observerType));
				return;
			}

			auto* world = static_cast<flecs::world*>(this->m_world);
			flecs::observer observer = world->observer<T>().event(event).each([callback, this](flecs::entity entity, T &component) {
				callback(Entity{this, entity.view().id()}, &component);
			});
		}

		/// Destroy an entity.
		/// @param entity Entity to destroy
		void DestroyEntity(Entity &&entity);

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

		[[nodiscard]] [[hush::export]]
		EntityId RegisterComponentRaw(const ComponentTraits::ComponentInfo &desc) const;

		template <typename... Components>
		Query<Components...> CreateQuery(RawQuery::ECacheMode cacheMode = RawQuery::ECacheMode::Default)
		{
			std::array<Entity::EntityId, sizeof...(Components)> components = {RegisterIfNeededSlow<Components>()...};

			auto rawQuery = CreateRawQuery(components, cacheMode);

			return Query<Components...>(std::move(rawQuery));
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

		[[nodiscard]]
		void *GetWorld() const
		{
			return m_world;
		}

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

		/// User systems
		std::vector<std::unique_ptr<ISystem>> m_userSystems;

		HushEngine *m_engine;

		void *m_world;
	};
} // namespace Hush
