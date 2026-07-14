#include <catch2/catch_test_macros.hpp>
#include <string_view>

#include "HMeta.hpp"

using namespace Hush;

TEST_CASE("HMeta JSON round-trip", "[hmeta]")
{
	SECTION("Serialize and deserialize a texture HMeta")
	{
		HMeta meta;
		meta.id = 12345;
		meta.assetType = "texture";
		meta.outputFormat = EAssetFormat::RGBA8_UNORM;
		meta.compression = ECompressionFormat::Zstd;
		meta.sourceHash = 0xABCDEF;
		meta.texture.sRGB = true;
		meta.texture.generateMipmaps = true;
		meta.texture.gpuCompression = "none";
		meta.texture.maxSize = 1024;

		auto jsonResult = meta.ToJson();
		REQUIRE(jsonResult.has_value());

		std::string json = jsonResult.value();
		REQUIRE_FALSE(json.empty());

		// Deserialize
		auto readResult = HMeta::FromJson(json);
		REQUIRE(readResult.has_value());

		auto &readBack = readResult.value();
		REQUIRE(readBack.version == HMeta::VERSION);
		REQUIRE(readBack.id == 12345);
		REQUIRE(readBack.assetType == "texture");
		REQUIRE(readBack.outputFormat == EAssetFormat::RGBA8_UNORM);
		REQUIRE(readBack.compression == ECompressionFormat::Zstd);
		REQUIRE(readBack.sourceHash == 0xABCDEF);
		REQUIRE(readBack.texture.sRGB == true);
		REQUIRE(readBack.texture.generateMipmaps == true);
		REQUIRE(readBack.texture.gpuCompression == "none");
		REQUIRE(readBack.texture.maxSize == 1024);
	}

	SECTION("Deserialize minimal JSON with defaults")
	{
		std::string_view json = R"({
			"version": 1,
			"id": 42,
			"assetType": "shader",
			"outputFormat": 32,
			"compression": "zstd",
			"importSettings": {
				"backends": [0],
				"entryPoints": ["main"]
			}
		})";

		auto result = HMeta::FromJson(json);
		REQUIRE(result.has_value());

		auto &meta = result.value();
		REQUIRE(meta.id == 42);
		REQUIRE(meta.assetType == "shader");
		REQUIRE(meta.outputFormat == EAssetFormat::Shader);
		REQUIRE(meta.compression == ECompressionFormat::Zstd);
		REQUIRE(meta.shader.backends.size() == 1);
		REQUIRE(meta.shader.backends[0] == EShaderBackend::WebGPU_WGSL);
		REQUIRE(meta.shader.entryPoints.size() == 1);
		REQUIRE(meta.shader.entryPoints[0] == "main");
		REQUIRE(meta.texture.sRGB == true); // default
	}

	SECTION("Invalid JSON returns error")
	{
		auto result = HMeta::FromJson("not json");
		REQUIRE_FALSE(result.has_value());
	}
}
