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
#include <array>
#include <algorithm>

namespace Hush::Reflection
{
	class FieldInfo
	{
	public:
		using EVariantError = Variant::EVariantError;

		using Setter = std::function<EVariantError(std::span<const VariantView>)>;
		using Getter = std::function<Result<Variant, EVariantError>(std::span<const VariantView>)>;

		FieldInfo(TypeId typeId, std::string name, Setter setter, Getter getter)
			: m_typeId(typeId),
			  m_name(std::move(name)),
			  m_setter(setter),
			  m_getter(getter)
		{
		}

		[[nodiscard]]
		TypeId GetTypeId() const
		{
			return m_typeId;
		}

		[[nodiscard]]
		const std::string &GetName() const
		{
			return m_name;
		}

		[[nodiscard]]
		Result<Variant, Variant::EVariantError> Get(std::span<const VariantView> args) const
		{
			if (m_getter != nullptr)
			{
				return m_getter(args);
			}
			return EVariantError::NonSameType;
		}

		template <typename... Args>
			requires(std::is_same_v<Args, VariantView> && ...)
		[[nodiscard]]
		Result<Variant, Variant::EVariantError> Get(std::initializer_list<VariantView> args) const
		{
			std::array<VariantView, sizeof...(args)> argArray;
			std::copy(args.begin(), args.end(), argArray.begin());

			return Get(argArray);
		}

		Result<Variant, Variant::EVariantError> Get(std::initializer_list<const VariantView> args) const
		{
			return Get(std::span(args));
		}

		[[nodiscard]]
		EVariantError Set(const std::span<const VariantView> args) const
		{
			if (m_setter != nullptr)
			{
				return m_setter(args);
			}
			return EVariantError::NonSameType;
		}

		[[nodiscard]]
		EVariantError Set(std::initializer_list<VariantView> args) const
		{
			return Set(std::span(args));
		}


	private:
		TypeId m_typeId;
		std::string m_name;
		Setter m_setter;
		Getter m_getter;
	};
} // namespace Hush::Reflection