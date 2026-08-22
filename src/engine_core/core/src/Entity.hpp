/*! \file Entity.hpp
	\author Alan Ramirez
	\date 2025-01-22
	\brief Scene entity
*/

#pragma once

#include "Assertions.hpp"
#include "NullTerminatedStringView.hpp"
#include "traits/EntityTraits.hpp"
#include "HushBindings.hpp"

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <string_view>
#include <Hushgen.hpp>

#include <reflection/Type.hpp>
#include <serialization/Serialization.hpp>
#include <serialization/Deserialization.hpp>
#include <type_traits>

#if __has_include("Entity.hushgen.hpp") && !defined(HUSH_HEADER_PARSING)
#include "Entity.hushgen.hpp"
#endif

namespace Hush
{
	class Scene;

	template <typename... Components>
	class Query;
	// Upper bound on sizeof(ecs_ref_t): 48 bytes on 64-bit targets. On 32 bit target (such as wasm32) the two trailing
	// pointers are 4 bytes each, so it is 40 there.
	constexpr size_t ECS_REF_SIZE = 48;
	constexpr size_t ECS_REF_ALIGN = 8; // ecs_ref_t holds uint64_t/pointers, so it needs 8-byte alignment.

	class Entity;

	class ComponentRef
	{
	public:
		void *GetDataRaw();

		[[nodiscard]]
		const void *GetDataRaw() const;

		template <class T>
		T *GetData()
		{
			return static_cast<T *>(GetDataRaw());
		}

		template <class T>
		const T *GetData() const
		{
			return static_cast<const T *>(GetDataRaw());
		}

		[[nodiscard]]
		uint64_t GetComponentId() const;

	private:
		alignas(ECS_REF_ALIGN) mutable std::array<std::byte, ECS_REF_SIZE> m_refInternal;
		// TODO: Make it a thread local variable
		void *m_world = nullptr;
		friend class Entity;
	};

	///
	/// Describes an entity in the scene.
	/// An entity is something that exists in the scene. It can have components attached to it.
	/// Entities are not meant to be created directly, but through the scene.
	///
	/// Creating components requires the component to be registered in the scene. This is done automatically when
	/// any of the component functions are called. If the component is not registered, it will be registered in the
	/// scene.
	///
	/// For scripting, this class exposes `*ComponentRaw` functions that are used to interact with components using
	/// raw component ids. These functions are not meant to be used directly, and, while you can, they're inconvenient.
	/// The `*Component` functions are the ones that should be used.
	class [[hush::export]] Entity
	{
	public:
		constexpr static size_t MAX_ENTITY_NAME_LENGTH = 32;
		using EntityId = std::uint64_t;
		constexpr static EntityId INVALID_ENTITY_ID = 0;

		/// @brief Component that holds the name of an entity
		struct [[hush::reflect]] Name
		{
			HUSH_GENERATED_BODY
		public:
			// NOLINTNEXTLINE

			Name() = default;

			Name(std::string_view name)
			{
				SetName(name);
			}

			void SetName(std::string_view name)
			{
				HUSH_COND_FAIL_MSG(name.size() <= MAX_ENTITY_NAME_LENGTH,
								   "Maximum character length for entity name was exceeded");
				size_t copyLength = std::min(name.size(), MAX_ENTITY_NAME_LENGTH);
				std::copy_n(name.data(), copyLength, this->m_name.data());
				// NOLINTNEXTLINE
				this->m_name[copyLength] = '\0';
				this->m_length = copyLength;
			}

			[[nodiscard]]
			std::string_view GetName() const
			{
				return {this->m_name.data(), this->m_length};
			}

		private:
			std::array<char, MAX_ENTITY_NAME_LENGTH + 1> m_name{}; // Handle null terminator!!!
			size_t m_length{};
		};
		explicit Entity(Scene *ownerScene, std::uint64_t entityId)
			: m_entityId(entityId),
			  m_ownerScene(ownerScene)
		{
		}

		/// Entity destructor. It does not destroy the entity. For that, use `Scene::DestroyEntity`.
		~Entity() noexcept = default;

		Entity(const Entity &) = delete;

		Entity &operator=(const Entity &) = delete;

		Entity(Entity &&other) noexcept
			: m_entityId(std::exchange(other.m_entityId, 0)),
			  m_ownerScene(std::exchange(other.m_ownerScene, nullptr))
		{
		}

		Entity &operator=(Entity &&other) noexcept
		{
			if (this != &other)
			{
				this->m_entityId = std::exchange(other.m_entityId, 0);
				this->m_ownerScene = std::exchange(other.m_ownerScene, nullptr);
			}

			return *this;
		}

		Entity() = default;

		/// Get a null entity. A null entity is an entity that does not exist in the scene. It can be used to represent
		/// an invalid entity.
		///
		/// @return A null entity.
		[[nodiscard]]
		static Entity Null()
		{
			return Entity{nullptr, 0};
		}

		/// Checks if the entity has a component of the given type.
		/// @tparam T Type of the component.
		/// @return True if the entity has the component, false otherwise.
		template <typename T>
		bool HasComponent()
		{
			EntityId componentId = RegisterIfNeededSlow<std::remove_cvref_t<T>>();

			return HasComponentRaw(componentId);
		}

		/// Add a component to the entity.
		/// If the component is already added, it will return a reference to the existing component.
		/// @tparam T Type of the component.
		/// @return Reference to the added component.
		template <typename T>
			requires std::is_default_constructible_v<T>
		std::remove_cvref_t<T> &AddComponent()
		{
			const EntityId componentId = RegisterIfNeededSlow<T>();

			return *static_cast<std::remove_cvref_t<T> *>(AddComponentRaw(componentId));
		}

		/// Emplace a component to the entity. If the component is already added, it will return a reference to the
		/// existing component.
		///
		/// @tparam T Type of the component.
		/// @tparam Args Arguments to pass to the constructor of the component.
		/// @param args Arguments to pass to the constructor of the component.
		/// @return Reference to the added component.
		template <typename T, typename... Args>
			requires std::is_constructible_v<T, Args...>
		std::remove_cvref_t<T> &EmplaceComponent(Args &&...args)
		{
			const EntityId componentId = RegisterIfNeededSlow<std::remove_cvref_t<T>>();

			bool isNew = false;

			auto *component = EmplaceComponentRaw(componentId, sizeof(T), isNew);

			if (!isNew)
			{
				return *static_cast<std::remove_cvref_t<T> *>(component);
			}

			return *new (component) std::remove_cvref_t<T>(std::forward<Args>(args)...);
		}

		/// Get a component from the entity.
		/// @tparam T Type of the component.
		/// @return Pointer to the component, or nullptr if the component is not found.
		template <typename T>
		std::remove_cvref_t<T> *GetComponent()
		{
			const EntityId componentId = RegisterIfNeededSlow<std::remove_cvref_t<T>>();

			return static_cast<std::remove_cvref_t<T> *>(GetComponentRaw(componentId));
		}

		/// Get a component from the entity.
		/// @tparam T Type of the component.
		/// @return Pointer to the component, or nullptr if the component is not found.
		template <typename T>
		const std::remove_cvref_t<T> *GetComponent() const
		{
			const EntityId componentId = RegisterIfNeededSlow<std::remove_cvref_t<T>>();

			return static_cast<const std::remove_cvref_t<T> *>(GetComponentRaw(componentId));
		}

		/// Remove a component from the entity.
		/// @tparam T Type of the component.
		/// @return True if the component was removed, false otherwise.
		template <typename T>
		bool RemoveComponent()
		{
			const EntityId entityId = RegisterIfNeededSlow<std::remove_cvref_t<T>>();

			return RemoveComponentRaw(entityId);
		}

		template <typename T>
		void SetComponentActive(bool active)
		{
			const EntityId entityId = RegisterIfNeededSlow<std::remove_cvref_t<T>>();

			SetComponentActiveRaw(entityId, active);
		}

		/// Register a component to the entity.
		/// @tparam T Type to register.
		template <typename T>
		void RegisterComponent() const
		{
			(void)RegisterIfNeededSlow<std::remove_cvref_t<T>>();
		}

		template <class T>
		ComponentRef CreateComponentReference()
		{
			const EntityId entityId = RegisterIfNeededSlow<std::remove_cvref_t<T>>();

			return CreateComponentReferenceRaw(entityId);
		}

		// Raw component functions. Mostly for internal use but also usable by bindings. They don't check if the
		// component is registered.

		/// Register a component.
		/// @param desc Component description.
		/// @return Id of the component.
		[[hush::export]] [[nodiscard]]
		EntityId RegisterComponentRaw(const ComponentTraits::ComponentInfo &desc) const;

		/// @brief Notifies a component has been modified, this is useful when you get a component through its raw
		/// pointer and you have an observer that's listening for changes
		void NotifyComponentModifiedRaw(Entity::EntityId componentId);

		void EmitEvent(Entity::EntityId event);

		ComponentRef CreateComponentReferenceRaw(EntityId componentId);

		/// Add a component to the entity.
		/// @param componentId Id of the component.
		/// @return Pointer to the component.
		[[hush::export]]
		void *AddComponentRaw(EntityId componentId);

		/// Get a component from the entity.
		/// @param componentId Id of the component.
		/// @return Pointer to the component.
		[[nodiscard]] [[hush::export]]
		void *GetComponentRaw(EntityId componentId);

		/// Get a component from the entity.
		/// @param componentId Id of the component.
		/// @return Pointer to the component.
		[[nodiscard]] [[hush::export]]
		void *GetComponentRaw(EntityId componentId) const;

		/// Check if the entity has a component.
		/// @param componentId Id of the component.
		/// @return True if the entity has the component, false otherwise.
		[[nodiscard]] [[hush::export]]
		bool HasComponentRaw(EntityId componentId);

		/// Emplace a component to the entity.
		/// If the component is already added, it will return a reference to the existing component.
		/// If the component is new, the user is in charge of constructing it. Not constructing it is undefined
		/// behavior.
		///
		/// @param componentId Id of the component.
		/// @param isNew Flag to indicate if the component is new. If it is new, user is in charge of constructing it.
		/// @param componentSize Size of the component.
		/// @return Pointer to the component.
		[[nodiscard]] [[hush::export]]
		void *EmplaceComponentRaw(EntityId componentId, size_t componentSize, bool &isNew);

		/// Remove a component from the entity.
		/// @param componentId Id of the component.
		/// @return True if the component was removed, false otherwise.
		[[hush::export]]
		bool RemoveComponentRaw(EntityId componentId);

		[[hush::export]]
		void SetComponentActiveRaw(EntityId componentId, bool active);

		/// Destroy an entity. This will remove all components from the entity and destroy it.
		/// This consumes the entity, so it should not be used after this function is called.
		/// @param entity Entity to destroy.
		static void Destroy(Entity &&entity);

		void SetParent(const Entity &parent);

		[[hush::export]]
		void AddChild(const Entity &child);

		[[nodiscard]]
		Entity GetParent() const;

		/// @brief Gets the child at the specified index, returns an invalid entity if none is found
		/// @param index Index of the child to get
		/// @returns The child entity at the index or INVALID_ENTITY
		[[nodiscard]]
		Entity GetChildAt(int32_t index) const;

		[[nodiscard]] [[hush::export]]
		int32_t GetChildCount() const;

		void EachChild(std::function<void(Entity &)> func) const;

		// Iterates each ID associated with this entity, includiding components and relationships
		void EachId(std::function<void(Entity::EntityId)> &&func);

		[[hush::export]]
		void AddRelationship(const Entity &relationship, const Entity &target);

		[[nodiscard]] [[hush::export]]
		EntityId GetId() const;

		[[nodiscard]]
		std::string_view GetKey() const;

		[[nodiscard]]
		inline bool IsValid() const
		{
			return this->m_entityId != INVALID_ENTITY_ID;
		}

		[[nodiscard]] [[hush::export]]
		bool IsAlive() const;

	private:
		friend class Scene;
		friend class Query<>;

		/// Register a component if it is not registered.
		/// @tparam T Type of the component.
		/// @return Id of the component.
		template <typename T>
		[[nodiscard]]
		EntityId RegisterIfNeededSlow() const
		{
			// First, get the entity id, and check if the component is registered.
			auto [status, componentId] = ComponentTraits::detail::GetEntityId<T>(GetSceneUniqueId());
			const ComponentTraits::ComponentInfo info = ComponentTraits::GetComponentInfo<T>();

			return InternalRegisterCppComponent(status, componentId, info);
		}

		[[nodiscard]]
		void *GetSceneWorld() const;

		[[nodiscard]]
		std::uint64_t GetSceneUniqueId() const;

		/// Check if a component is registered.
		/// @param componentId Id of the component.
		/// @return True if the component is registered, false otherwise.
		[[nodiscard]]
		bool IsComponentRegistered(EntityId componentId) const;

		/// Register a C++ component. C++ components are special because they use a cache in the scene to
		/// avoid registering the same component multiple times.
		/// @param desc Component description.
		/// @return ID of the component.
		[[nodiscard]]
		EntityId InternalRegisterCppComponent(ComponentTraits::detail::EEntityRegisterStatus registerStatus,
											  std::uint64_t *id, const ComponentTraits::ComponentInfo &desc) const;

		/// Get the id of a component from the cache.
		/// @param name Name of the component.
		/// @return Id of the component, or std::nullopt if the component is not found.
		[[nodiscard]]
		std::optional<EntityId> InternalCachedComponentId(NullTerminatedStringView name) const;

		/// Id of the entity
		EntityId m_entityId{};

		/// Scene that owns this entity
		Scene *m_ownerScene = nullptr;
	};

} // namespace Hush
