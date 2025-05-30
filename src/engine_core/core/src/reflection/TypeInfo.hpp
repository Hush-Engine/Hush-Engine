/*! \file TypeInfo.hpp
	\author Alan Ramirez
	\date 2025-04-20
	\brief TypeInfo implementation
*/

#pragma once
#include "TypeTraits.hpp"
#include "FunctionInfo.hpp"
#include "FieldInfo.hpp"

#include <vector>
#include <optional>

namespace Hush::Reflection
{

	class TypeInfo
	{
	public:
		TypeInfo(TypeId id = {})
			: m_id(id)
		{
		}

		[[nodiscard]]
		TypeId GetId() const
		{
			return m_id;
		}

		void AddFunction(const FunctionInfo &function)
		{
			m_functions.push_back(function);
		}

		void AddField(const FieldInfo &field)
		{
			m_fields.push_back(field);
		}

		[[nodiscard]]
		std::span<const FunctionInfo> GetFunctions() const
		{
			return m_functions;
		}

		[[nodiscard]]
		std::span<const FieldInfo> GetFields() const
		{
			return m_fields;
		}

		[[nodiscard]]
		std::optional<std::reference_wrapper<const FieldInfo>> GetField(std::string_view name) const
		{
			for (auto &field : m_fields)
			{
				if (field.GetName() == name)
				{
					return std::ref(field);
				}
			}
			return std::nullopt;
		}

		[[nodiscard]]
		const std::string &GetName() const
		{
			return m_name;
		}

		void SetName(std::string_view name)
		{
			this->m_name = name;
		}

		[[nodiscard]]
		std::size_t GetSize() const
		{
			return m_size;
		}
		void SetSize(std::size_t size)
		{
			this->m_size = size;
		}

		[[nodiscard]]
		std::size_t GetAlignment() const
		{
			return m_alignment;
		}

		void SetAlignment(std::size_t alignment)
		{
			this->m_alignment = alignment;
		}

		[[nodiscard]]
		Result<Variant, FunctionInfo::EFunctionInfoError> CreateInstance(std::span<VariantView> args) const
		{
			for (const auto &constructor : m_constructors)
			{
				if (constructor.IsCallableWith(args))
				{
					return constructor.Call(args);
				}
			}

			return FunctionInfo::EFunctionInfoError::NonMatchingArgs;
		}

		template <typename... Args>
		[[nodiscard]]
		Result<Variant, FunctionInfo::EFunctionInfoError> CreateInstance(Args &&...args) const
		{
			std::array<VariantView, sizeof...(Args)> argsArray{std::forward<Args>(args)...};
			return CreateInstance(argsArray);
		}

		void SetConstructors(std::vector<FunctionInfo> &&constructors)
		{
			m_constructors = std::move(constructors);
		}

		void SetFunctions(std::vector<FunctionInfo> &&functions)
		{
			m_functions = std::move(functions);
		}

		void SetFields(std::vector<FieldInfo> &&fields)
		{
			m_fields = std::move(fields);
		}

	private:
		TypeId m_id;
		std::vector<FunctionInfo> m_constructors;
		std::vector<FunctionInfo> m_functions;
		std::vector<FieldInfo> m_fields;
		std::string m_name;
		std::size_t m_size{0};
		std::size_t m_alignment{0};
	};

	template <typename T>
	[[nodiscard]]
	inline TypeInfo GetTypeInfo()
	{
		static_assert(false, "TypeInfo is not defined for this type");
		return {};
	}

	template <ReflectedType T>
	[[nodiscard]]
	inline TypeInfo GetTypeInfo()
	{
		return T::GetTypeInfo();
	}

	template <>
	[[nodiscard]]
	inline TypeInfo GetTypeInfo<void>()
	{
		return TypeInfo{};
	}

} // namespace Hush::Reflection