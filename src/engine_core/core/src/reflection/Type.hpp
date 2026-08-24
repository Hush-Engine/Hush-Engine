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
#include <vector>

#include "TypeTraits.hpp"
#include "TypeId.hpp"
#include "TypeInfo.hpp"
#include "ModuleHandle.hpp"
#include "Platform.hpp"

#include <crypto/Hashing.hpp>

#ifdef HUSH_COMPILER_MSVC
#pragma warning(push)
#pragma warning(disable : 5030) // Attribute not recognized
#endif

// RegisterClass might be defined by Windows, so undefine it to avoid conflicts with generated code.
#ifdef RegisterClass
#undef RegisterClass
#endif

namespace Hush::Reflection
{
	/// Result of registering a class into the reflection database.
	enum class ERegisterClassError : std::uint8_t
	{
		/// The type was registered.
		None = 0,
		/// A different type with the same type id was already registered.
		/// The already registered type is kept.
		DuplicateType = 1,
	};

	class ReflectionDB
	{
	public:
		template <typename T>
		struct RegisterClassBuilder
		{
		public:
			explicit RegisterClassBuilder(ReflectionDB *reflectionDB)
				: m_reflectionDB(reflectionDB)
			{
			}

			RegisterClassBuilder &AddConstructor(FunctionInfo constructor)
			{
				m_constructor.push_back(std::move(constructor));

				return *this;
			}

			RegisterClassBuilder &AddInPlaceConstructor(TypeInfo::InPlaceCtor ctor)
			{
				m_inPlaceCtors.emplace_back(ctor);

				return *this;
			}

			RegisterClassBuilder &AddFunction(FunctionInfo function)
			{
				m_functions.push_back(std::move(function));

				return *this;
			}

			RegisterClassBuilder &SizeOf(std::size_t size)
			{
				m_size = size;

				return *this;
			}

			RegisterClassBuilder &AlignmentOf(std::size_t alignment)
			{
				m_alignment = alignment;

				return *this;
			}

			RegisterClassBuilder &AddProperty(FieldInfo property)
			{
				m_fields.push_back(std::move(property));

				return *this;
			}

			/// Adds a metadata pair that is stored in the type info.
			RegisterClassBuilder &AddMetadata(std::string key, std::string value)
			{
				m_metadata.insert_or_assign(std::move(key), std::move(value));

				return *this;
			}

			/// Registers the type into the database.
			/// @param module Module that owns the type. The engine module owns
			/// built-in types.
			/// @return The registration result.
			ERegisterClassError Register(ModuleHandle module = ENGINE_MODULE_HANDLE)
			{
				TypeInfo typeInfo(GetTypeId<T>());
				typeInfo.SetName(T::TypeName());
				typeInfo.SetSize(sizeof(T));
				typeInfo.SetAlignment(alignof(T));
				typeInfo.SetConstructors(std::move(m_constructor));
				typeInfo.SetFunctions(std::move(m_functions));
				typeInfo.SetFields(std::move(m_fields));
				typeInfo.SetInPlaceCtors(std::move(m_inPlaceCtors));
				typeInfo.SetMetadata(std::move(m_metadata));

				return m_reflectionDB->RegisterClass(std::move(typeInfo), module);
			}

		private:
			std::vector<FunctionInfo> m_constructor;
			std::vector<TypeInfo::InPlaceCtor> m_inPlaceCtors;
			std::vector<FunctionInfo> m_functions;
			std::vector<FieldInfo> m_fields;
			MetadataMap m_metadata;
			ReflectionDB *m_reflectionDB;
			std::size_t m_size{0};
			std::size_t m_alignment{0};
		};

		/// Registers a type into the database.
		/// @param typeInfo Type information to register. The owner of the type
		/// is set to the given module.
		/// @param module Module that owns the type.
		/// @return None when the type was registered, DuplicateType when a type
		/// with the same id was already registered.
		ERegisterClassError RegisterClass(TypeInfo typeInfo, ModuleHandle module = ENGINE_MODULE_HANDLE)
		{
			std::unique_lock lock(m_mutex);
			TypeId id = typeInfo.GetId();
			if (m_types.find(id) != m_types.end())
			{
				return ERegisterClassError::DuplicateType;
			}
			typeInfo.SetOwner(module);
			m_types.insert_or_assign(id, std::move(typeInfo));
			m_moduleTypes[module].push_back(id);
			return ERegisterClassError::None;
		}

		/// Removes every type that belongs to the given module. The engine
		/// module cannot be unregistered.
		///
		/// Callers must make sure that nothing uses the removed types anymore.
		/// Any pointer returned by GetTypeInfo for the removed types stops
		/// being valid after this call.
		///
		/// @param module Module whose types are removed.
		/// @return Number of removed types.
		std::size_t UnregisterModule(ModuleHandle module)
		{
			if (module == ENGINE_MODULE_HANDLE)
			{
				return 0;
			}

			std::unique_lock lock(m_mutex);
			const auto it = m_moduleTypes.find(module);
			if (it == m_moduleTypes.end())
			{
				return 0;
			}

			std::size_t removed = 0;
			for (TypeId id : it->second)
			{
				removed += m_types.erase(id);
			}
			m_moduleTypes.erase(it);
			return removed;
		}

		/// Gets a copy of the type ids that belong to the given module.
		[[nodiscard]]
		std::vector<TypeId> GetModuleTypes(ModuleHandle module) const
		{
			std::shared_lock lock(m_mutex);
			const auto it = m_moduleTypes.find(module);
			if (it == m_moduleTypes.end())
			{
				return {};
			}
			return it->second;
		}

		/// Returns the number of registered types.
		[[nodiscard]]
		std::size_t GetTypeCount() const
		{
			std::shared_lock lock(m_mutex);
			return m_types.size();
		}

		[[nodiscard]]
		bool HasType(TypeId id) const
		{
			std::shared_lock lock(m_mutex);
			return m_types.contains(id);
		}

		[[nodiscard]]
		std::optional<ModuleHandle> GetTypeOwner(TypeId id) const
		{
			std::shared_lock lock(m_mutex);
			const auto it = m_types.find(id);
			return it == m_types.end() ? std::nullopt : std::optional<ModuleHandle>(it->second.GetOwner());
		}

		[[nodiscard]]
		std::optional<std::string> GetTypeName(TypeId id) const
		{
			std::shared_lock lock(m_mutex);
			const auto it = m_types.find(id);
			return it == m_types.end() ? std::nullopt : std::optional<std::string>(it->second.GetName());
		}

		[[nodiscard]]
		bool HasTypeMetadata(TypeId id, std::string_view key) const
		{
			std::shared_lock lock(m_mutex);
			const auto it = m_types.find(id);
			return it != m_types.end() && it->second.HasMetadata(key);
		}

		[[nodiscard]]
		std::optional<std::string> GetTypeMetadata(TypeId id, std::string_view key) const
		{
			std::shared_lock lock(m_mutex);
			const auto it = m_types.find(id);
			if (it == m_types.end())
			{
				return std::nullopt;
			}
			const std::optional<std::string_view> value = it->second.GetMetadata(key);
			return value.has_value() ? std::optional<std::string>(*value) : std::nullopt;
		}

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

		[[nodiscard]]
		const TypeInfo *GetTypeInfo(std::string_view name) const
		{
			std::shared_lock lock(m_mutex);
			const TypeId id = TypeId{Hush::Hashing::Fnv1a64(name)};

			const auto it = m_types.find(id);
			if (it != m_types.end())
			{
				return &it->second;
			}

			return nullptr;
		}

		template <ReflectedType T>
		RegisterClassBuilder<T> RegisterClass()
		{
			RegisterClassBuilder<T> builder(this);

			return builder;
		}

	private:
		mutable std::shared_mutex m_mutex;
		std::unordered_map<TypeId, TypeInfo> m_types;

		/// Type ids owned by each module, used to remove everything a module
		/// registered when the module goes away.
		std::unordered_map<ModuleHandle, std::vector<TypeId>> m_moduleTypes;
	};

} // namespace Hush::Reflection
