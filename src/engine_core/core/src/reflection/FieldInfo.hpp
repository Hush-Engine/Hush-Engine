/*! \file FieldInfo.hpp
	\author Alan Ramirez
	\date 2025-04-20
	\brief Field info implementation
*/

#pragma once
#include "Variant.hpp"
#include "Metadata.hpp"

#include <span>
#include <initializer_list>
#include <functional>
#include <array>
#include <algorithm>

namespace Hush::Reflection
{
	class FieldInfo : public MetadataHolder
	{
	public:
		using EVariantError = Variant::EVariantError;

		using Setter = std::function<EVariantError(std::span<const VariantView>)>;
		using Getter = std::function<Result<Variant, EVariantError>(std::span<const VariantView>)>;

		FieldInfo(TypeId typeId, std::string name, Setter setter, Getter getter, uint64_t offset = 0,
				  MetadataMap metadata = {})
			: m_typeId(typeId),
			  m_name(std::move(name)),
			  m_setter(std::move(setter)),
			  m_getter(std::move(getter)),
			  m_offset(offset)
		{
			SetMetadata(std::move(metadata));
		}

		[[nodiscard]]
		TypeId GetTypeId() const
		{
			return m_typeId;
		}

		[[nodiscard]]
		std::string_view GetName() const
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

		[[nodiscard]]
		uint64_t GetOffset() const
		{
			return m_offset;
		}

	private:
		TypeId m_typeId;
		std::string m_name;
		Setter m_setter;
		Getter m_getter;
		uint64_t m_offset{0};
	};
} // namespace Hush::Reflection