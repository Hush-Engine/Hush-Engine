#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <vector>
#include <span>

#include "HAsset.hpp"

using namespace Hush;

TEST_CASE("HAsset round-trip", "[hasset]")
{
	SECTION("Write and read back a simple HAsset")
	{
		HAsset asset;
		asset.header.format = EAssetFormat::RGBA8_UNORM;
		asset.header.compression = ECompressionFormat::None;
		asset.header.uncompressedSize = 64;
		asset.header.compressedSize = 64;
		asset.header.contentHash = 0xDEADBEEF;

		// Add texture extra
		HTextureExtra texExtra;
		texExtra.width = 4;
		texExtra.height = 4;
		texExtra.depth = 1;
		texExtra.mipCount = 1;
		texExtra.gpuFormat = 1;
		asset.extra.resize(sizeof(texExtra));
		std::memcpy(asset.extra.data(), &texExtra, sizeof(texExtra));

		// Add a small payload
		asset.payload.resize(64);
		for (size_t i = 0; i < 64; ++i)
			asset.payload[i] = static_cast<std::byte>(i);

		// Serialize
		std::vector<std::byte> buffer;
		HAsset::Write(buffer, asset);

		REQUIRE(buffer.size() == asset.SerializedSize());

		// Deserialize
		auto result = HAsset::Read(buffer);
		REQUIRE(result.has_value());

		auto &readBack = result.value();
		REQUIRE(readBack.header.magic == HASSET_MAGIC);
		REQUIRE(readBack.header.headerVersion == HASSET_VERSION);
		REQUIRE(readBack.header.format == EAssetFormat::RGBA8_UNORM);
		REQUIRE(readBack.header.uncompressedSize == 64);
		REQUIRE(readBack.header.contentHash == 0xDEADBEEF);
		REQUIRE(readBack.extra.size() == sizeof(HTextureExtra));
		REQUIRE(readBack.payload.size() == 64);

		// Verify payload content
		for (size_t i = 0; i < 64; ++i)
			REQUIRE(readBack.payload[i] == static_cast<std::byte>(i));

		// Verify extra content
		HTextureExtra readExtra;
		std::memcpy(&readExtra, readBack.extra.data(), sizeof(HTextureExtra));
		REQUIRE(readExtra.width == 4);
		REQUIRE(readExtra.height == 4);
	}

	SECTION("Invalid data returns nullopt")
	{
		std::vector<std::byte> bad(10);
		auto result = HAsset::Read(bad);
		REQUIRE_FALSE(result.has_value());
	}

	SECTION("Wrong magic returns nullopt")
	{
		std::vector<std::byte> bad(sizeof(HAssetHeader));
		bad[0] = std::byte{0xFF};
		auto result = HAsset::Read(bad);
		REQUIRE_FALSE(result.has_value());
	}
}
