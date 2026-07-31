#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <limits>
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
		wgsl.bytecode = {std::byte{'v'}, std::byte{'s'}, std::byte{'_'}, std::byte{'m'},
						 std::byte{'a'}, std::byte{'i'}, std::byte{'n'}};

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

TEST_CASE("HShader rejects malformed input", "[hshader]")
{
	SECTION("Large backendCount with small totalSize returns nullopt")
	{
		std::vector<std::byte> buffer(sizeof(HShaderHeader) + 4);
		HShaderHeader hdr;
		hdr.magic = HSHADER_MAGIC;
		hdr.version = HSHADER_VERSION;
		hdr.backendCount = 65535;
		hdr.totalSize = static_cast<uint32_t>(buffer.size());
		std::memcpy(buffer.data(), &hdr, sizeof(HShaderHeader));

		REQUIRE_FALSE(HShader::Read(buffer).has_value());
	}

	SECTION("Backend bytecode out of bounds returns nullopt")
	{
		HShader shader;
		shader.header.magic = HSHADER_MAGIC;
		shader.header.version = HSHADER_VERSION;

		HShader::BackendData wgsl;
		wgsl.entryNames.push_back("vertexMain");
		wgsl.bytecode = {std::byte{'v'}};

		shader.backends.push_back({.backendType = EShaderBackend::WebGPU_WGSL, .stageCount = 1});
		shader.backendData.push_back(std::move(wgsl));

		std::vector<std::byte> buffer;
		HShader::Write(buffer, shader);

		const size_t dataOffsetPos = sizeof(HShaderHeader) + offsetof(BackendEntry, dataOffset);
		const uint64_t huge = std::numeric_limits<uint64_t>::max();
		std::memcpy(buffer.data() + dataOffsetPos, &huge, sizeof(huge));

		REQUIRE_FALSE(HShader::Read(buffer).has_value());
	}
}
