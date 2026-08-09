#pragma once

#include "AssetFormat.hpp"
#include <cstdint>
#include <span>
#include <vector>
#include <optional>

namespace Hush
{

	static constexpr uint32_t HASSET_MAGIC = 0x54534148; // 'HAST'
	static constexpr uint16_t HASSET_VERSION = 1;

#pragma pack(push, 1)
	struct HAssetHeader
	{
		uint32_t magic = HASSET_MAGIC;
		uint16_t headerVersion = HASSET_VERSION;
		uint16_t flags = 0; // bit0 = encrypted (reserved)
		EAssetFormat format = EAssetFormat::Unknown;
		ECompressionFormat compression = ECompressionFormat::None;
		uint64_t uncompressedSize = 0;
		uint64_t compressedSize = 0;
		uint32_t extraSize = 0;
		uint32_t contentHash = 0; // Fnv1a of uncompressed payload
	};

	struct HTextureExtra
	{
		uint32_t width = 0;
		uint32_t height = 0;
		uint32_t depth = 1;
		uint32_t mipCount = 1;
		uint32_t arrayLayers = 1;
		uint32_t gpuFormat = 0; // ETextureFormat value
	};
#pragma pack(pop)

	static_assert(sizeof(HAssetHeader) == 40, "HAssetHeader must be 40 bytes");

	struct HAsset
	{
		HAssetHeader header;
		std::vector<std::byte> extra;
		std::vector<std::byte> payload; // compressed or raw

		/// Write the complete HAsset blob (header + extra + payload) into out.
		static void Write(std::vector<std::byte> &out, const HAsset &asset);

		/// Read a complete HAsset blob from a span.
		/// Returns std::nullopt on invalid magic/version.
		static std::optional<HAsset> Read(std::span<const std::byte> data);

		/// Total serialized size.
		[[nodiscard]]
		size_t SerializedSize() const;
	};

} // namespace Hush
