#include <catch2/catch_test_macros.hpp>
#include <vector>
#include <cstdint>
#include <cstring>

#include "AssetCooker.hpp"
#include "HAsset.hpp"

using namespace Hush;

// A valid 16x16 RGBA PNG, all-white (solid color so the zstd payload is well
// under the raw 16*16*4 bytes). Generated with zlib; CRCs are correct so stb_image
// decodes it. Kept 16x16 (not 2x2) so ECompressionFormat::Zstd unambiguously shrinks
// the payload — a 2x2 (16-byte) payload is smaller than a zstd frame's overhead.
static constexpr int kTestImageDim = 16;
static const unsigned char kTestPng[] = {
	0x89, 0x50, 0x4E, 0x47, 0x0D, 0x0A, 0x1A, 0x0A, 0x00, 0x00, 0x00, 0x0D,
	0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x10,
	0x08, 0x06, 0x00, 0x00, 0x00, 0x1F, 0xF3, 0xFF, 0x61, 0x00, 0x00, 0x00,
	0x16, 0x49, 0x44, 0x41, 0x54, 0x78, 0xDA, 0x63, 0xF8, 0x4F, 0x21, 0x60,
	0x18, 0x35, 0x60, 0xD4, 0x80, 0x51, 0x03, 0x86, 0x8B, 0x01, 0x00, 0x5D,
	0x78, 0xFC, 0x2E, 0xAD, 0x21, 0xA9, 0x5E, 0x00, 0x00, 0x00, 0x00, 0x49,
	0x45, 0x4E, 0x44, 0xAE, 0x42, 0x60, 0x82
};

TEST_CASE("ImageCooker decodes PNG to HAsset", "[imagecooker]")
{
	AssetCooker cooker;
	cooker.RegisterBuiltins();

	std::span<const std::byte> input(reinterpret_cast<const std::byte *>(kTestPng), sizeof(kTestPng));

	HMeta meta;
	meta.assetType = "texture";
	meta.outputFormat = EAssetFormat::RGBA8_UNORM;
	meta.compression = ECompressionFormat::None;

	CookContext ctx;
	ctx.sourceVPath = "test.png";

	auto result = cooker.CookToBlob(input, EFileExtension::PNG, meta, ctx);
	REQUIRE(result.has_value());

	auto &blob = result.value();

	// Read back the HAsset
	auto asset = HAsset::Read(blob);
	REQUIRE(asset.has_value());

	REQUIRE(asset->header.format == EAssetFormat::RGBA8_UNORM);
	REQUIRE(asset->header.compression == ECompressionFormat::None);
	REQUIRE(asset->header.uncompressedSize == kTestImageDim * kTestImageDim * 4); // RGBA8
	REQUIRE(asset->payload.size() == kTestImageDim * kTestImageDim * 4);

	// Verify extra
	REQUIRE(asset->extra.size() == sizeof(HTextureExtra));

	HTextureExtra texExtra;
	std::memcpy(&texExtra, asset->extra.data(), sizeof(HTextureExtra));
	REQUIRE(texExtra.width == kTestImageDim);
	REQUIRE(texExtra.height == kTestImageDim);
}

TEST_CASE("ImageCooker compresses with zstd", "[imagecooker]")
{
	AssetCooker cooker;
	cooker.RegisterBuiltins();

	std::span<const std::byte> input(reinterpret_cast<const std::byte *>(kTestPng), sizeof(kTestPng));

	HMeta meta;
	meta.assetType = "texture";
	meta.outputFormat = EAssetFormat::RGBA8_UNORM;
	meta.compression = ECompressionFormat::Zstd;

	CookContext ctx;
	ctx.sourceVPath = "test.png";

	auto result = cooker.CookToBlob(input, EFileExtension::PNG, meta, ctx);
	REQUIRE(result.has_value());

	auto &blob = result.value();

	auto asset = HAsset::Read(blob);
	REQUIRE(asset.has_value());

	REQUIRE(asset->header.compression == ECompressionFormat::Zstd);
	REQUIRE(asset->header.compressedSize < asset->header.uncompressedSize);
	REQUIRE(asset->header.uncompressedSize == kTestImageDim * kTestImageDim * 4);
}
