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
		REQUIRE(meta.shader.entryPoints[0].name == "main");
		REQUIRE(meta.shader.entryPoints[0].stage.empty());
		REQUIRE(meta.texture.sRGB == true); // default
	}

	SECTION("Entry points with explicit stages round-trip")
	{
		HMeta meta;
		meta.assetType = "shader";
		meta.outputFormat = EAssetFormat::Shader;
		meta.shader.entryPoints.push_back(HMeta::ShaderEntryPoint{.name = "vsMain", .stage = "vertex"});
		meta.shader.entryPoints.push_back(HMeta::ShaderEntryPoint{.name = "fsMain", .stage = "fragment"});
		meta.shader.entryPoints.push_back(HMeta::ShaderEntryPoint{.name = "main", .stage = {}});

		auto jsonResult = meta.ToJson();
		REQUIRE(jsonResult.has_value());

		auto readResult = HMeta::FromJson(jsonResult.value());
		REQUIRE(readResult.has_value());

		auto &readBack = readResult.value();
		REQUIRE(readBack.shader.entryPoints.size() == 3);
		REQUIRE(readBack.shader.entryPoints[0].name == "vsMain");
		REQUIRE(readBack.shader.entryPoints[0].stage == "vertex");
		REQUIRE(readBack.shader.entryPoints[1].name == "fsMain");
		REQUIRE(readBack.shader.entryPoints[1].stage == "fragment");
		REQUIRE(readBack.shader.entryPoints[2].name == "main");
		REQUIRE(readBack.shader.entryPoints[2].stage.empty());
	}

	SECTION("Integral scaleFactor is accepted")
	{
		std::string_view json = R"({
			"version": 1,
			"id": 7,
			"assetType": "model",
			"importSettings": {
				"scaleFactor": 2
			}
		})";

		auto result = HMeta::FromJson(json);
		REQUIRE(result.has_value());
		REQUIRE(result.value().model.scaleFactor == 2.0f);
	}

	SECTION("Invalid JSON returns error")
	{
		auto result = HMeta::FromJson("not json");
		REQUIRE_FALSE(result.has_value());
	}
}
