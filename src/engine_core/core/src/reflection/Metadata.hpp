/*! \file Metadata.hpp
	\author Alan Ramirez
	\date 2025-11-21
	\brief Key/value metadata attached to reflected types, fields and functions
*/

#pragma once

#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace Hush::Reflection
{
	/// Key/value metadata attached to a reflected element. Both keys and
	/// values are stored as owned strings so metadata stays valid even when
	/// the module that registered it is unloaded.
	using MetadataMap = std::unordered_map<std::string, std::string>;

	/// Well-known metadata key set on types marked with [[hush::builtin]].
	inline constexpr std::string_view METADATA_KEY_BUILTIN = "hush.builtin";

	/// Well-known metadata key set on types marked with [[hush::component]].
	inline constexpr std::string_view METADATA_KEY_COMPONENT = "hush.component";

	/// Well-known metadata key set on types marked with [[hush::system]].
	inline constexpr std::string_view METADATA_KEY_SYSTEM = "hush.system";

	/// Small helper mixin that adds metadata storage to a reflection class.
	class MetadataHolder
	{
	public:
		/// Adds or replaces a metadata value.
		/// @param key Metadata key.
		/// @param value Metadata value.
		void AddMetadata(std::string key, std::string value)
		{
			m_metadata.insert_or_assign(std::move(key), std::move(value));
		}

		/// Gets a metadata value by key, or an empty optional when it does not exist.
		/// @param key Metadata key.
		[[nodiscard]]
		std::optional<std::string_view> GetMetadata(std::string_view key) const
		{
			const auto it = m_metadata.find(std::string(key));
			if (it == m_metadata.end())
			{
				return std::nullopt;
			}
			return it->second;
		}

		/// Returns true when the metadata key exists.
		[[nodiscard]]
		bool HasMetadata(std::string_view key) const
		{
			return m_metadata.find(std::string(key)) != m_metadata.end();
		}

		/// Returns all metadata pairs.
		[[nodiscard]]
		const MetadataMap &GetMetadataMap() const
		{
			return m_metadata;
		}

		/// Replaces all metadata pairs.
		void SetMetadata(MetadataMap metadata)
		{
			m_metadata = std::move(metadata);
		}

	private:
		MetadataMap m_metadata;
	};
} // namespace Hush::Reflection
