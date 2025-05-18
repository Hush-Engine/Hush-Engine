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

namespace Hush::Reflection
{

	class TypeInfo
	{
	public:
		/**
		 * @brief Constructs a TypeInfo object with an optional type identifier.
		 *
		 * @param id The unique identifier for the type. Defaults to an empty TypeId if not provided.
		 */
		TypeInfo(TypeId id = {})
			: m_id(id)
		{
		}

		/**
		 * @brief Returns the unique identifier associated with this type.
		 *
		 * @return The TypeId representing this type.
		 */
		[[nodiscard]]
		TypeId GetId() const
		{
			return m_id;
		}

		/**
		 * @brief Adds a function to the type's metadata.
		 *
		 * Appends the provided function information to the list of functions associated with this type.
		 */
		void AddFunction(const FunctionInfo &function)
		{
			m_functions.push_back(function);
		}

		/**
		 * @brief Adds a field to the type's reflection metadata.
		 *
		 * Appends the provided field information to the list of fields associated with this type.
		 *
		 * @param field The field metadata to add.
		 */
		void AddField(const FieldInfo &field)
		{
			m_fields.push_back(field);
		}

		/**
		 * @brief Returns a read-only view of the functions associated with this type.
		 *
		 * @return std::span<const FunctionInfo> Span of function metadata for the type.
		 */
		[[nodiscard]]
		std::span<const FunctionInfo> GetFunctions() const
		{
			return m_functions;
		}

		/**
		 * @brief Returns a read-only view of the type's reflected fields.
		 *
		 * @return A span of constant FieldInfo objects representing the fields of the type.
		 */
		[[nodiscard]]
		std::span<const FieldInfo> GetFields() const
		{
			return m_fields;
		}

		/**
		 * @brief Returns the name of the type.
		 *
		 * @return Reference to the type's name string.
		 */
		[[nodiscard]]
		const std::string &GetName() const
		{
			return m_name;
		}

		/**
		 * @brief Sets the name of the type represented by this TypeInfo.
		 *
		 * @param name The new name to assign to the type.
		 */
		void SetName(std::string_view name)
		{
			this->m_name = name;
		}

		/**
		 * @brief Returns the size of the type in bytes.
		 *
		 * @return The size of the type represented by this TypeInfo.
		 */
		[[nodiscard]]
		std::size_t GetSize() const
		{
			return m_size;
		}
		/**
		 * @brief Sets the size of the type in bytes.
		 *
		 * @param size The size of the type in bytes.
		 */
		void SetSize(std::size_t size)
		{
			this->m_size = size;
		}

		/**
		 * @brief Returns the alignment requirement of the type in bytes.
		 *
		 * @return The alignment of the type.
		 */
		[[nodiscard]]
		std::size_t GetAlignment() const
		{
			return m_alignment;
		}

		/**
		 * @brief Sets the alignment requirement for the type in bytes.
		 *
		 * @param alignment The alignment value to assign.
		 */
		void SetAlignment(std::size_t alignment)
		{
			this->m_alignment = alignment;
		}

		/**
		 * @brief Attempts to create an instance of the reflected type using the provided arguments.
		 *
		 * Iterates through available constructors and invokes the first one that matches the argument signature.
		 *
		 * @param args Arguments to be passed to the constructor.
		 * @return Result containing the constructed Variant on success, or a FunctionInfo::EFunctionInfoError if no matching constructor is found.
		 */
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
		/**
		 * @brief Creates an instance of the reflected type using the provided arguments.
		 *
		 * Forwards the given arguments to the type's constructors and attempts to instantiate the type by matching the argument signature. Returns a result containing the created instance as a Variant, or an error if no matching constructor is found.
		 *
		 * @tparam Args Types of the constructor arguments.
		 * @param args Arguments to pass to the constructor.
		 * @return Result<Variant, FunctionInfo::EFunctionInfoError> The constructed instance or an error code.
		 */
		[[nodiscard]]
		Result<Variant, FunctionInfo::EFunctionInfoError> CreateInstance(Args &&...args) const
		{
			std::array<VariantView, sizeof...(Args)> argsArray{std::forward<Args>(args)...};
			return CreateInstance(argsArray);
		}

		/**
		 * @brief Replaces the list of constructors associated with this type.
		 *
		 * Moves the provided vector of constructors into the internal storage, overwriting any existing constructors.
		 */
		void SetConstructors(std::vector<FunctionInfo> &&constructors)
		{
			m_constructors = std::move(constructors);
		}

		/**
		 * @brief Replaces the stored list of functions with the provided vector.
		 *
		 * Moves the given vector of FunctionInfo objects into the internal function storage, replacing any existing functions.
		 */
		void SetFunctions(std::vector<FunctionInfo> &&functions)
		{
			m_functions = std::move(functions);
		}

		/**
		 * @brief Replaces the stored field metadata with the provided fields.
		 *
		 * Moves the given vector of FieldInfo objects into the internal storage, replacing any existing field metadata.
		 */
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
	/**
	 * @brief Triggers a compile-time error if called for a type without defined TypeInfo.
	 *
	 * This primary template causes a static assertion failure, indicating that reflection metadata is not available for the requested type.
	 */
	[[nodiscard]]
	inline TypeInfo GetTypeInfo()
	{
		static_assert(false, "TypeInfo is not defined for this type");
		return {};
	}

	template <ReflectedType T>
	/**
	 * @brief Retrieves the reflection metadata for a type that implements the ReflectedType concept.
	 *
	 * Calls the static GetTypeInfo() method on the type T to obtain its TypeInfo.
	 *
	 * @return TypeInfo Reflection metadata for the type T.
	 */
	[[nodiscard]]
	inline TypeInfo GetTypeInfo()
	{
		return T::GetTypeInfo();
	}

	template <>
	/**
	 * @brief Returns an empty TypeInfo object for the void type.
	 *
	 * This specialization provides a default-constructed TypeInfo for cases where type reflection is requested for void.
	 * @return TypeInfo An empty TypeInfo instance.
	 */
	[[nodiscard]]
	inline TypeInfo GetTypeInfo<void>()
	{
		return TypeInfo{};
	}

} // namespace Hush::Reflection