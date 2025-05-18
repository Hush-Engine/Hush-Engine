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
			explicit RegisterClassBuilder(ReflectionDB *reflectionDB)
				: m_reflectionDB(reflectionDB)
			{
			}

			RegisterClassBuilder &AddConstructor(FunctionInfo constructor)
			{
				m_constructor.push_back(std::move(constructor));

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
			const TypeId id = {Hush::Hashing::Fnv1a64(name)};

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
	};

} // namespace Hush::Reflection