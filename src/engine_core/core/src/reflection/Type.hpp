/*! \file Type.hpp
	\author Alan Ramirez
	\date 2025-04-15
	\brief Hush Engine Reflection Type
*/

#pragma once

#include <unordered_map>
#include <shared_mutex>
#include <utility>
#include <span>

#include "TypeTraits.hpp"
#include "TypeId.hpp"
#include "TypeInfo.hpp"

#include <crypto/Hashing.hpp>

namespace Hush::Reflection
{
	class ReflectionDB
	{
	public:
		template <typename T>
		struct RegisterClassBuilder
		{
		public:
			/**
			 * @brief Constructs a RegisterClassBuilder for registering type metadata.
			 *
			 * @param reflectionDB Pointer to the ReflectionDB instance where the type will be registered.
			 */
			explicit RegisterClassBuilder(ReflectionDB *reflectionDB)
				: m_reflectionDB(reflectionDB)
			{
			}

			/**
			 * @brief Adds a constructor to the type registration builder.
			 *
			 * Appends the provided constructor metadata to the list of constructors for the type being registered.
			 *
			 * @param constructor Metadata describing a constructor for the type.
			 * @return Reference to this builder for method chaining.
			 */
			RegisterClassBuilder &AddConstructor(FunctionInfo constructor)
			{
				m_constructor.push_back(std::move(constructor));

				return *this;
			}

			/**
			 * @brief Adds a member function to the type being registered.
			 *
			 * @param function Metadata describing the function to add.
			 * @return Reference to this builder for method chaining.
			 */
			RegisterClassBuilder &AddFunction(FunctionInfo function)
			{
				m_functions.push_back(std::move(function));

				return *this;
			}

			/**
			 * @brief Sets the size value for the type being registered.
			 *
			 * This value is stored in the builder but is not used during registration, as the actual size is determined by sizeof(T).
			 *
			 * @param size The size in bytes to associate with the type.
			 * @return Reference to this builder for method chaining.
			 */
			RegisterClassBuilder &SizeOf(std::size_t size)
			{
				m_size = size;

				return *this;
			}

			/**
			 * @brief Sets the alignment value for the type being registered.
			 *
			 * @param alignment The alignment in bytes to associate with the type.
			 * @return Reference to this builder for method chaining.
			 */
			RegisterClassBuilder &AlignmentOf(std::size_t alignment)
			{
				m_alignment = alignment;

				return *this;
			}

			/**
			 * @brief Adds a property (field) to the type being registered.
			 *
			 * @param property Metadata describing the field to add.
			 * @return Reference to this builder for method chaining.
			 */
			RegisterClassBuilder &AddProperty(FieldInfo property)
			{
				m_fields.push_back(std::move(property));

				return *this;
			}

			/**
			 * @brief Registers the type T and its metadata with the reflection database.
			 *
			 * Constructs a TypeInfo object for type T using the accumulated constructors, functions, and fields, then registers it in the associated ReflectionDB instance.
			 */
			void Register()
			{
				TypeInfo typeInfo(GetTypeId<T>());
				typeInfo.SetName(T::TypeName());
				typeInfo.SetSize(sizeof(T));
				typeInfo.SetAlignment(alignof(T));
				typeInfo.SetConstructors(std::move(m_constructor));
				typeInfo.SetFunctions(std::move(m_functions));
				typeInfo.SetFields(std::move(m_fields));

				m_reflectionDB->RegisterClass(std::move(typeInfo));
			}

		private:
			std::vector<FunctionInfo> m_constructor;
			std::vector<FunctionInfo> m_functions;
			std::vector<FieldInfo> m_fields;
			ReflectionDB *m_reflectionDB;
			std::size_t m_size{0};
			std::size_t m_alignment{0};
		};

		/**
		 * @brief Registers a new type in the reflection database if it is not already present.
		 *
		 * If a type with the same TypeId already exists, the registration is ignored.
		 *
		 * @param typeInfo The type metadata to register.
		 */
		void RegisterClass(TypeInfo typeInfo)
		{
			std::unique_lock lock(m_mutex);
			TypeId id = typeInfo.GetId();
			if (m_types.find(id) != m_types.end())
			{
				return;
			}
			m_types.insert_or_assign(id, std::move(typeInfo));
		}

		/**
		 * @brief Retrieves type metadata for a given type ID.
		 *
		 * @param id The unique identifier of the type.
		 * @return Pointer to the corresponding TypeInfo if found, or nullptr if the type is not registered.
		 */
		[[nodiscard]]
		const TypeInfo *GetTypeInfo(TypeId id) const
		{
			std::shared_lock lock(m_mutex);
			const auto it = m_types.find(id);
			if (it != m_types.end())
			{
				return &it->second;
			}

			return nullptr;
		}

		/**
		 * @brief Retrieves type metadata by type name.
		 *
		 * Looks up and returns a pointer to the TypeInfo associated with the given type name, or nullptr if not found.
		 *
		 * @param name The name of the type to look up.
		 * @return Pointer to the corresponding TypeInfo, or nullptr if the type is not registered.
		 */
		[[nodiscard]]
		const TypeInfo *GetTypeInfo(std::string_view name) const
		{
			std::shared_lock lock(m_mutex);
			const TypeId id = {Hush::Hashing::Fnv1a64(name)};

			const auto it = m_types.find(id);
			if (it != m_types.end())
			{
				return &it->second;
			}

			return nullptr;
		}

		template <ReflectedType T>
		/**
		 * @brief Begins building a registration for the reflected type T.
		 *
		 * @return A RegisterClassBuilder<T> instance for configuring and registering type metadata for T.
		 */
		RegisterClassBuilder<T> RegisterClass()
		{
			RegisterClassBuilder<T> builder(this);

			return builder;
		}

	private:
		mutable std::shared_mutex m_mutex;
		std::unordered_map<TypeId, TypeInfo> m_types;
	};

} // namespace Hush::Reflection