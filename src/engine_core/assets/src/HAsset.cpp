#include "HAsset.hpp"
#include <cstring>

namespace Hush
{

	void HAsset::Write(std::vector<std::byte> &out, const HAsset &asset)
	{
		const size_t headerSize = sizeof(HAssetHeader);
		const size_t extraSize = asset.extra.size();
		const size_t payloadSize = asset.payload.size();

		out.resize(headerSize + extraSize + payloadSize);

		HAssetHeader hdr = asset.header;
		hdr.extraSize = static_cast<uint32_t>(extraSize);
		std::memcpy(out.data(), &hdr, headerSize);
		if (extraSize > 0)
		{
			std::memcpy(out.data() + headerSize, asset.extra.data(), extraSize);
		}
		if (payloadSize > 0)
		{
			std::memcpy(out.data() + headerSize + extraSize, asset.payload.data(), payloadSize);
		}
	}

	std::optional<HAsset> HAsset::Read(std::span<const std::byte> data)
	{
		if (data.size() < sizeof(HAssetHeader))
		{
			return std::nullopt;
		}

		HAssetHeader header;
		std::memcpy(&header, data.data(), sizeof(HAssetHeader));

		if (header.magic != HASSET_MAGIC || header.headerVersion != HASSET_VERSION)
		{
			return std::nullopt;
		}

		// Subtraction-based checks: header fields are untrusted, so avoid addition overflow.
		const size_t payloadRegion = data.size() - sizeof(HAssetHeader);
		if (header.extraSize > payloadRegion)
		{
			return std::nullopt;
		}
		if (header.compressedSize > payloadRegion - header.extraSize)
		{
			return std::nullopt;
		}

		HAsset asset;
		asset.header = header;

		if (header.extraSize > 0)
		{
			asset.extra.resize(header.extraSize);
			std::memcpy(asset.extra.data(), data.data() + sizeof(HAssetHeader), header.extraSize);
		}

		if (header.compressedSize > 0)
		{
			asset.payload.resize(header.compressedSize);
			std::memcpy(asset.payload.data(), data.data() + sizeof(HAssetHeader) + header.extraSize,
						header.compressedSize);
		}

		return asset;
	}

	size_t HAsset::SerializedSize() const
	{
		return sizeof(HAssetHeader) + extra.size() + payload.size();
	}

} // namespace Hush
