/*! \file Query.hpp
	\author Alan Ramirez
	\date 2025-02-01
	\brief Query implementation
*/

#pragma once

#include <Entity.hpp>
#include <algorithm>
#include <array>
#include <cstdint>
#include <span>
#include <traits/EntityTraits.hpp>
#include <HushBindings.hpp>

namespace Hush
{
	class Scene;
	class RawQuery;

	///
	/// Low-level query for entities.
	/// This allows getting raw void pointers to components.
	/// To iterate, this class provides a \ref Hush::RawQuery::GetIterator function that returns an iterator.
	/// For more information about the iterator, see \ref QueryIterator.
	class [[hush::export]] RawQuery
	{
		RawQuery(Scene *scene, void *query);

	public:
		/// Cache mode for the query.
		enum class [[hush::export]] ECacheMode
		{
			Default,
			Auto,
			All,
			None
		};

		/// Component access mode.
		enum class [[hush::export]] EComponentAccess
		{
			ReadOnly = 0,
			WriteOnly = 1,
			ReadWrite = 2,
			Default = ReadWrite
		};

		/// Raw query iterator.
		/// This allows iterating over the entities in the query.
		///
		/// This iterator works by giving direct access to the arrays of components in the query.
		/// So, this doesn't give an array of |Component1, Component2, ...|, but instead gives n arrays of |Component1,
		/// Component1, ...|, |Component2, Component2, ...|, etc. Keep this in mind when using this iterator.
		///
		/// For end-users (not scripting), it is recommended to use the \ref Hush::Query class instead.
		struct [[hush::export]] QueryIterator
		{
			QueryIterator(Scene *scene) noexcept
				: m_scene(scene)
			{
			}

			QueryIterator(const QueryIterator &) = delete;
			QueryIterator &operator=(const QueryIterator &) = delete;

			QueryIterator(QueryIterator &&rhs) noexcept;
			QueryIterator &operator=(QueryIterator &&rhs) noexcept;

			~QueryIterator();

			/// Move to the next entity.
			/// @return True if there is a next entity, false otherwise.
			[[nodiscard, hush::export]]
			bool Next();

			/// Skip the current entity.
			[[hush::export]]
			void Skip();

			/// Check if the iterator has been finished.
			/// @return True if the iterator has been finished. False otherwise.
			[[nodiscard, hush::export]]
			bool Finished() const;

			/// Gives the number of entities in the current table.
			/// This number is updated when using \ref Next.
			///
			/// @return The number of entities in the query.
			[[nodiscard, hush::export]]
			std::size_t Size() const;

			/// Get an array of components at the given index.
			/// Index is given by the order of the components in the query. For instance,
			/// if the query was creating with a list of components [Position, Velocity], then
			/// Position would be at index 0 and Velocity at index 1.
			///
			/// The length of the array is given by \ref Size.
			///
			/// @param index Index of the component.
			/// @param size Size of the component.
			/// @return Pointer to the component.
			[[nodiscard, hush::export]]
			void *const GetComponentAt(std::int8_t index, std::size_t size) const;

			/// Get the entity id at the given index.
			/// @param index Index of the entity. This must be in range [0, Size()).
			/// @return Entity id at the given index.
			[[nodiscard, hush::export]]
			std::uint64_t GetEntityAt(std::size_t index) const;

			Scene *GetScene() const
			{
				return m_scene;
			}

		private:
			friend class RawQuery;

			/// Size of the ecs_query_iter_t struct.
			static constexpr std::size_t ECS_ITER_SIZE = 384;
			/// Alignment of the ecs_query_iter_t struct.
			static constexpr std::size_t ECS_ITER_ALIGNMENT = 8;

			alignas(ECS_ITER_ALIGNMENT) std::array<std::byte, ECS_ITER_SIZE> m_iterData{};
			Scene *m_scene = nullptr;
			bool m_hasBeenDestroyed = false;
		};

		static constexpr std::uint32_t MAX_COMPONENTS = 32;

		RawQuery() = delete;
		RawQuery(const RawQuery &) = delete;
		RawQuery &operator=(const RawQuery &) = delete;

		/// Move constructor.
		/// @param rhs Other query to move from.
		RawQuery(RawQuery &&rhs) noexcept;

		/// Move assignment operator.
		/// @param rhs Other query to move from.
		/// @return self
		RawQuery &operator=(RawQuery &&rhs) noexcept;

		~RawQuery() noexcept;

		/// Get the scene where the query is running.
		/// @return Scene where the query is running.
		[[nodiscard, hush::export]]
		Scene *GetScene() const noexcept;

		/// Get an iterator to iterate over the entities in the query.
		/// @return Iterator to iterate over the entities in the query.
		[[nodiscard, hush::export]]
		QueryIterator GetIterator();

	private:
		friend class Scene;

		void *m_query;
		Scene *m_scene;
	};

	namespace impl
	{
		class QueryImpl
		{
			using EntityId = std::uint64_t;

		public:
			QueryImpl(RawQuery query) noexcept
				: m_rawQuery(std::move(query))
			{
			}

			QueryImpl(const QueryImpl &) = delete;
			QueryImpl &operator=(const QueryImpl &) = delete;

			QueryImpl(QueryImpl &&rhs) noexcept
				: m_rawQuery(std::move(rhs.m_rawQuery))
			{
			}

			QueryImpl &operator=(QueryImpl &&rhs) noexcept
			{
				if (this != &rhs)
				{
					m_rawQuery = std::move(rhs.m_rawQuery);
				}

				return *this;
			}

		protected:
			~QueryImpl() = default;

			RawQuery &GetRawQuery()
			{
				return m_rawQuery;
			}

			const RawQuery &GetRawQuery() const
			{
				return m_rawQuery;
			}

			[[nodiscard]]
			EntityId InternalRegisterCppComponent(ComponentTraits::detail::EEntityRegisterStatus registerStatus,
												  std::uint64_t *id, const ComponentTraits::ComponentInfo &desc);

		private:
			RawQuery m_rawQuery;
		};
	} // namespace impl

	/// Query for entities. This wraps the raw query and allows getting components in a type-safe way.
	///
	/// It allows iterating using two methods:
	/// 1. Accessing directly to the component arrays, to do this, use the `begin` and `end` functions. This also can be
	///    iterated using range-based for loops.
	///
	/// 2. Using the `Each` function, which allows applying a function to each entity in the query. This is a simple way
	///    of iterating over the entities.
	///
	/// Example using the first method:
	///
	/// ```cpp
	/// Query<Position, Velocity> query = scene.CreateQuery<Position, Velocity>();
	/// auto it = query.begin();
	/// for (; it != query.end(); ++it)
	/// {
	///     auto [positions, velocities] = *it;
	///     for (auto position : positions)
	///     {
	///         // Do something with the position
	///     }
	///
	///     for (auto velocity : velocities)
	///     {
	///         // Do something with the velocity
	///     }
	///
	///     // Or
	///     for (std::size_t i = 0; i < it.Size(); ++i)
	///     {
	///         Position& position = positions[i];
	///         Velocity& velocity = velocities[i];
	///         Hush::Entity entity = it.GetEntity(i);
	///     }
	/// }
	/// ```
	///
	/// Example using the second method:
	///
	/// ```cpp
	/// Query<Position, Velocity> query = scene.CreateQuery<Position, Velocity>();
	///
	/// query.Each([](Position &position, Velocity &velocity) { });
	/// query.Each([](Entity::EntityId entityId, Position &position, Velocity &velocity) { });
	/// query.Each([](Hush::Entity &entity, Position &position, Velocity &velocity) { });
	/// ```
	///
	/// @tparam Components Components to query.
	template <typename... Components>
	class Query : public impl::QueryImpl
	{
		using EntityId = std::uint64_t;

	public:
		using ECacheMode = RawQuery::ECacheMode;

		using ComponentTuple = std::tuple<std::span<std::remove_reference_t<Components>>...>;
		using ConstComponentTuple = std::tuple<std::span<std::add_const_t<std::remove_reference_t<Components>>>...>;

		/// Constructor.
		/// @param query Raw query.
		Query(RawQuery query) noexcept
			: QueryImpl(std::move(query))
		{
		}

		/// This is a sentinel iterator that is used to signal the end of the query.
		struct SentinelQueryIterator
		{
		};

		/// Query iterator. This allows using for-range loops to iterate over the array of components.
		///
		struct QueryIterator
		{
			QueryIterator(RawQuery::QueryIterator iter)
				: m_iter(std::move(iter))
			{
			}

			QueryIterator(const QueryIterator &) = delete;
			QueryIterator &operator=(const QueryIterator &) = delete;

			QueryIterator(QueryIterator &&rhs) noexcept
				: m_iter(std::move(rhs.m_iter))
			{
			}

			QueryIterator &operator=(QueryIterator &&rhs) noexcept
			{
				if (this != &rhs)
				{
					m_iter = std::move(rhs.m_iter);
				}

				return *this;
			}

			~QueryIterator() = default;

			/// Move to the next table.
			///
			/// @return self
			QueryIterator &operator++()
			{
				if (m_iter.Finished())
				{
					return *this;
				}
				(void)m_iter.Next();
				return *this;
			}

			/// Get the next table of components.
			/// For instance, for a Query<Position, Velocity>, this will return a std::tuple<std::span<Position>,
			/// std::span<Velocity>>.
			///
			/// @return A std::tuple of spans of the components.
			[[nodiscard]]
			ComponentTuple operator*()
			{
				return GetComponents(std::make_index_sequence<sizeof...(Components)>{});
			}

			/// Get the next table of components.
			/// For instance, for a const Query<Position, Velocity>, this will return a std::tuple<std::span<const
			/// Position>, std::span<const Velocity>>.
			///
			/// @return A std::tuple of spans of the components.
			[[nodiscard]]
			ConstComponentTuple operator*() const
			{
				return GetComponents(std::make_index_sequence<sizeof...(Components)>{});
			}

			/// Returns true if the iterator is finished.
			///
			/// @return True if the iterator is finished, false otherwise.
			[[nodiscard]]
			bool operator==(const SentinelQueryIterator &) const
			{
				return m_iter.Finished();
			}

			/// Returns true if the iterator is not finished.
			/// @return True if the iterator is not finished, false otherwise.
			[[nodiscard]]
			bool operator!=(const SentinelQueryIterator &) const
			{
				return !m_iter.Finished();
			}

			/// Get the underlying raw iterator.
			///
			/// Note: Advancing the raw iterator will also advance this iterator.
			///
			/// @return The raw iterator.
			[[nodiscard]]
			RawQuery::QueryIterator &GetRawIterator()
			{
				return m_iter;
			}

			/// Get the number of entities in the current table.
			/// @return The number of entities in the current table.
			[[nodiscard]]
			std::size_t Size() const
			{
				return m_iter.Size();
			}

			/// Get the entity id at the given index. Range is [0, Size()).
			/// @param index Index of the entity.
			/// @return Entity id at the given index.
			[[nodiscard]]
			EntityId GetEntityId(std::size_t index) const
			{
				return m_iter.GetEntityAt(index);
			}

			/// Get the entity at the given index. Range is [0, Size()).
			/// @param index Index of the entity.
			/// @return Entity at the given index.
			[[nodiscard]]
			Hush::Entity GetEntity(std::size_t index) const
			{
				return Hush::Entity(m_iter.GetScene(), GetEntityId(index));
			}

		private:
			friend struct SentinelQueryIterator;

			/// Get a tuple of spans of the components.
			///
			/// @tparam I Index sequence.
			/// @return Tuple of spans of the components.
			template <std::size_t... I>
			[[nodiscard]]
			ComponentTuple GetComponents(std::index_sequence<I...>)
			{
				return std::make_tuple(std::span<std::remove_reference_t<Components>>(
					static_cast<std::remove_reference_t<Components> *>(
						m_iter.GetComponentAt(I, sizeof(std::remove_reference_t<Components>))),
					m_iter.Size())...);
			}

			/// Get a tuple of spans of the components.
			///
			/// @tparam I Index sequence.
			/// @return Tuple of spans of the components.
			template <std::size_t... I>
			[[nodiscard]]
			ConstComponentTuple GetComponents(std::index_sequence<I...>) const
			{
				return std::make_tuple(std::span<std::add_const_t<std::remove_reference_t<Components>>>(
					static_cast<std::add_const_t<std::remove_reference_t<Components>> *>(
						m_iter.GetComponentAt(I, sizeof(std::remove_reference_t<Components>))),
					m_iter.Size())...);
			}

			RawQuery::QueryIterator m_iter;
		};

		/// Get an iterator to iterate over the entities in the query.
		/// @return Iterator to iterate over the entities in the query.
		[[nodiscard]]
		QueryIterator begin()
		{
			auto queryIter = GetRawQuery().GetIterator();
			// Force the first iteration to get the first entity.
			auto wrappedIter = QueryIterator(std::move(queryIter));
			++wrappedIter;
			return wrappedIter;
		}

		/// Get the sentinel iterator. This is used to check if the iterator is finished.
		/// @return SentinelQueryIterator
		[[nodiscard]]
		SentinelQueryIterator end()
		{
			return SentinelQueryIterator{};
		}

		/// Const version of the begin function.
		/// @return Const query iterator
		[[nodiscard]]
		QueryIterator begin() const
		{
			return QueryIterator(*this, 0);
		}

		/// Const version of the end function.
		/// @return SentinelQueryIterator
		[[nodiscard]]
		SentinelQueryIterator end() const
		{
			return SentinelQueryIterator{};
		}

		/// Apply a function to each entity in the query.
		/// This version of the function will only pass the components as arguments. Order is as the query was created.
		/// @tparam Func Function type.
		/// @param func Function to apply to each entity.
		template <typename Func>
			requires std::is_invocable_v<Func, std::add_lvalue_reference_t<Components>...>
		void Each(Func &&func)
		{
			auto it = begin();
			for (; it != end(); ++it)
			{
				const std::size_t size = it.Size();
				ComponentTuple components = *it;

				for (std::size_t i = 0; i < size; ++i)
				{
					// Get the i-th component of each span. To do this, we convert it to a tuple of references and then
					// apply it.
					auto component = std::apply(
						[&i](auto &&...components) {
							return std::tie<std::add_lvalue_reference_t<Components>...>(components[i]...);
						},
						components);

					std::apply(func, component);
				}
			}
		}

		/// Apply a function to each entity in the query.
		/// This version of the function also provides the entity id as the first argument.
		/// @tparam Func Function type.
		/// @param func Function to apply to each entity.
		template <typename Func>
			requires std::is_invocable_v<Func, EntityId, std::add_lvalue_reference_t<Components>...>
		void Each(Func &&func)
		{
			auto it = begin();
			for (; it != end(); ++it)
			{
				const std::size_t size = it.Size();
				ComponentTuple components = *it;

				for (std::size_t i = 0; i < size; ++i)
				{
					// Get the i-th component of each span. To do this, we convert it to a tuple of references and then
					// apply it.
					auto entityId = it.GetRawIterator().GetEntityAt(i);
					auto component = std::apply(
						[&i](auto &&...components) {
							return std::tie<std::add_lvalue_reference_t<Components>...>(components[i]...);
						},
						components);

					auto finalTuple = std::tuple_cat(std::make_tuple(entityId), component);
					std::apply(func, finalTuple);
				}
			}
		}

		/// Apply a function to each entity in the query.
		/// This version of the function will pass the entity as the first argument.
		/// @tparam Func Function type.
		/// @param func Function to apply to each entity.
		template <typename Func>
			requires std::is_invocable_v<Func, Entity &, std::add_lvalue_reference_t<Components>...>
		void Each(Func &&func)
		{
			auto it = begin();
			for (; it != end(); ++it)
			{
				const std::size_t size = it.Size();
				ComponentTuple components = *it;

				for (std::size_t i = 0; i < size; ++i)
				{
					// Get the i-th component of each span. To do this, we convert it to a tuple of references and then
					// apply it.
					auto entityId = it.GetRawIterator().GetEntityAt(i);
					auto component = std::apply(
						[&i](auto &&...components) {
							return std::tie<std::add_lvalue_reference_t<Components>...>(components[i]...);
						},
						components);

					auto finalTuple =
						std::tuple_cat(std::make_tuple(Entity(GetRawQuery().GetScene(), entityId)), component);
					std::apply(func, finalTuple);
				}
			}
		}

	private:
		template <typename T>
		[[nodiscard]]
		EntityId RegisterIfNeededSlow() const
		{
			// First, get the entity id, and check if the component is registered.
			auto [status, componentId] = ComponentTraits::detail::GetEntityId<T>(GetRawQuery().GetScene());
			const ComponentTraits::ComponentInfo info = ComponentTraits::GetComponentInfo<T>();

			return InternalRegisterCppComponent(status, componentId, info);
		}
	};

} // namespace Hush