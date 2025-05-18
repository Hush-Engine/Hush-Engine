/*! \file FieldInfo.hpp
	\author Alan Ramirez
	\date 2025-04-20
	\brief Field info implementation
*/

#pragma once
#include "Variant.hpp"

#include <span>
#include <initializer_list>
#include <functional>

namespace Hush::Reflection
{
	class FieldInfo
	{
	public:
		using EVariantError = Variant::EVariantError;

		using Setter = std::function<EVariantError(std::span<VariantView>)>;
		using Getter = std::function<Result<Variant, EVariantError>(std::span<VariantView>)>;

		/**
		 * @brief Constructs a FieldInfo object with the specified type, name, setter, and getter.
		 *
		 * @param typeId The type identifier of the field.
		 * @param name The name of the field.
		 * @param setter Function to set the field's value.
		 * @param getter Function to get the field's value.
		 */
		FieldInfo(TypeId typeId, std::string name, Setter setter, Getter getter)
			: m_typeId(typeId),
			  m_name(std::move(name)),
			  m_setter(setter),
			  m_getter(getter)
		{
		}

		/**
		 * @brief Returns the type identifier of the field.
		 *
		 * @return The TypeId representing the field's type.
		 */
		[[nodiscard]]
		TypeId GetTypeId() const
		{
			return m_typeId;
		}

		/**
		 * @brief Returns the name of the reflected field.
		 *
		 * @return Reference to the field's name string.
		 */
		[[nodiscard]]
		const std::string &GetName() const
		{
			return m_name;
		}

		/**
		 * @brief Retrieves the value of the field using the provided arguments.
		 *
		 * Invokes the stored getter function with the given arguments to obtain the field's value. If no getter is set, returns an error code indicating a type mismatch.
		 *
		 * @param args Arguments required by the getter, typically including the object instance and any additional parameters.
		 * @return Result containing the field value as a Variant on success, or an EVariantError on failure.
		 */
		[[nodiscard]]
		Result<Variant, Variant::EVariantError> Get(const std::span<VariantView> args) const
		{
			if (m_getter != nullptr)
			{
				return m_getter(args);
			}
			return EVariantError::NonSameType;
		}

		/**
		 * @brief Retrieves the value of the field using an initializer list of arguments.
		 *
		 * Converts the provided initializer list of VariantView arguments into an array and invokes the field's getter function.
		 *
		 * @param args Arguments required by the getter, provided as an initializer list.
		 * @return Result containing the field value as a Variant, or an error code if retrieval fails.
		 */
		[[nodiscard]]
		Result<Variant, Variant::EVariantError> Get(std::initializer_list<VariantView> args) const
		{
			std::array<VariantView, sizeof (args)> argArray;
			std::copy(args.begin(), args.end(), argArray.begin());

			return Get(argArray);
		}

		/**
		 * @brief Sets the value of the field using the provided arguments.
		 *
		 * Invokes the stored setter function with the given arguments if available. Returns an error code if the setter is not set.
		 *
		 * @param args Arguments to be passed to the setter, typically including the target object and the new value.
		 * @return EVariantError Result of the set operation, or EVariantError::NonSameType if the setter is not defined.
		 */
		[[nodiscard]]
		EVariantError Set(const std::span<VariantView> args) const
		{
			if (m_setter != nullptr)
			{
				return m_setter(args);
			}
			return EVariantError::NonSameType;
		}

		template <typename... Args> requires (std::is_same_v<Args, VariantView> && ...)
		/**
		 * @brief Sets the field's value using a variadic list of VariantView arguments.
		 *
		 * Packs the provided arguments into an array and invokes the field's setter function.
		 *
		 * @tparam Args Variadic arguments, each of type VariantView.
		 * @return EVariantError Error code indicating the result of the set operation.
		 */
		[[nodiscard]]
		EVariantError Set(Args... args) const
		{
			std::array argArray{args...};
			return Set(argArray);
		}

	private:
		TypeId m_typeId;
		std::string m_name;
		Setter m_setter;
		Getter m_getter;
	};
} // namespace Hush::Reflection