/*! \file FieldInfo.hpp
	\author Alan Ramirez
	\date 2025-04-20
	\brief Field info implementation
*/

#pragma once
#include "Variant.hpp"
#include "Metadata.hpp"
#include "reflection/TypeId.hpp"
#include "reflection/TypeInfo.hpp"

#include <cstdint>
#include <span>
#include <initializer_list>
#include <functional>
#include <array>
#include <algorithm>

#include <Hushgen.hpp>

#if __has_include("FieldInfo.hushgen.hpp") && !defined(HUSH_HEADER_PARSING)
#include "FieldInfo.hushgen.hpp"
#endif

#include "HushBindings.hpp"

namespace Hush::Reflection
{

	// Not putting them inside the class because it trips the exporter into exporting this typedef too
	using CSetter = uint8_t (*)(const uint8_t *, const uint8_t *);
	using CGetter = uint8_t (*)(const uint8_t *, ForeignVariant *outValue);
	using EVariantError = Variant::EVariantError;

	class [[hush::export]] FieldInfo : public MetadataHolder
	{
	public:
		using Setter = std::function<Variant::EVariantError(std::span<const VariantView>)>;
		using Getter = std::function<Result<Variant, Variant::EVariantError>(std::span<const VariantView>)>;

		// Fn pointers intended to be used with the foreign modules, these are less safe, for sure but
		// we circunvent VariantView this way, I guess BACKLOG: Port this using VariantView
		// TODO: Make this a little safer using std::span instead of raw pointers, this relies on the module knowing
		// each instance and field's size

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

		/// @brief Marshallable version of the FieldInfo constructor to expose it to the foreign lang bindings
		/// For now, foregin fields
		[[hush::export]]
		static FieldInfo MakeFieldInfo(uint64_t typeId, std::string_view name, CSetter setter, CGetter getter,
									   uint64_t offset)
		{
			return FieldInfo(
				{typeId}, std::string(name),
				[setter](std::span<const VariantView> data) {
					if (data.size() != 2)
					{
						return EVariantError::NonSameType;
					}
					auto *instance = reinterpret_cast<uint8_t *>(data[0].GetRaw(data[0].GetTypeId()));
					const auto *newValue = reinterpret_cast<const uint8_t *>(data[1].GetRaw(data[1].GetTypeId()));

					auto err = (EVariantError)setter(instance, newValue);
					return err;
				},
				[getter](std::span<const VariantView> data) -> Result<Variant, EVariantError> {
					if (data.size() != 1)
					{
						return EVariantError::NonSameType;
					}
					// The getter only has one param, the instance
					const auto *instance = reinterpret_cast<const uint8_t *>(data[0].GetRaw(data[0].GetTypeId()));
					ForeignVariant result{};
					auto err = (EVariantError)getter(instance, &result);
					if (err != EVariantError::None)
					{
						return err;
					}
					return std::move(result.GetInnerVariant());
				},
				offset);
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
