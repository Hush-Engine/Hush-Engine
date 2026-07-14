#include <catch2/catch_test_macros.hpp>
#include <vector>
#include <string>

#include "HShader.hpp"

using namespace Hush;

TEST_CASE("HShader round-trip", "[hshader]")
{
	SECTION("Write and read a WGSL shader")
	{
		HShader shader;
		shader.header.magic = HSHADER_MAGIC;
		shader.header.version = HSHADER_VERSION;

		HShader::BackendData wgsl;
		wgsl.entryNames.push_back("vertexMain");
		wgsl.bytecode = {std::byte{'v'}, std::byte{'s'}, std::byte{'_'}, std::byte{'m'}, std::byte{'a'}, std::byte{'i'}, std::byte{'n'}};

		shader.backends.push_back({.backendType = EShaderBackend::WebGPU_WGSL, .stageCount = 1});
		shader.backendData.push_back(std::move(wgsl));

		std::vector<std::byte> buffer;
		HShader::Write(buffer, shader);

		REQUIRE(buffer.size() == shader.SerializedSize());

		auto result = HShader::Read(buffer);
		REQUIRE(result.has_value());

		auto &readBack = result.value();
		REQUIRE(readBack.header.magic == HSHADER_MAGIC);
		REQUIRE(readBack.header.version == HSHADER_VERSION);
		REQUIRE(readBack.header.backendCount == 1);
		REQUIRE(readBack.backends.size() == 1);
		REQUIRE(readBack.backends[0].backendType == EShaderBackend::WebGPU_WGSL);
		REQUIRE(readBack.backends[0].stageCount == 1);
		REQUIRE(readBack.backendData.size() == 1);
		REQUIRE(readBack.backendData[0].entryNames.size() == 1);
		REQUIRE(readBack.backendData[0].entryNames[0] == "vertexMain");
		REQUIRE(readBack.backendData[0].bytecode.size() == 7);
	}

	SECTION("Invalid data returns nullopt")
	{
		std::vector<std::byte> bad(4);
		auto result = HShader::Read(bad);
		REQUIRE_FALSE(result.has_value());
	}
}
