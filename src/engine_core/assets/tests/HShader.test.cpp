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
		wgsl.stages.push_back(HShader::StageData{.stage = 0, // Vertex
												 .entryName = "vertexMain",
												 .codeOffset = 0,
												 .codeSize = 7});
		wgsl.stages.push_back(HShader::StageData{.stage = 1, // Fragment
												 .entryName = "fragmentMain",
												 .codeOffset = 7,
												 .codeSize = 7});
		wgsl.bytecode = {std::byte{'v'}, std::byte{'s'}, std::byte{'_'}, std::byte{'m'}, std::byte{'a'},
						 std::byte{'i'}, std::byte{'n'}, std::byte{'f'}, std::byte{'s'}, std::byte{'_'},
						 std::byte{'m'}, std::byte{'a'}, std::byte{'i'}, std::byte{'n'}};

		shader.backends.push_back({.backendType = EShaderBackend::WebGPU_WGSL, .stageCount = 2});
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
		REQUIRE(readBack.backends[0].stageCount == 2);
		REQUIRE(readBack.backendData.size() == 1);
		REQUIRE(readBack.backendData[0].stages.size() == 2);
		REQUIRE(readBack.backendData[0].stages[0].stage == 0);
		REQUIRE(readBack.backendData[0].stages[0].entryName == "vertexMain");
		REQUIRE(readBack.backendData[0].stages[0].codeOffset == 0);
		REQUIRE(readBack.backendData[0].stages[0].codeSize == 7);
		REQUIRE(readBack.backendData[0].stages[1].stage == 1);
		REQUIRE(readBack.backendData[0].stages[1].entryName == "fragmentMain");
		REQUIRE(readBack.backendData[0].stages[1].codeOffset == 7);
		REQUIRE(readBack.backendData[0].stages[1].codeSize == 7);
		REQUIRE(readBack.backendData[0].bytecode.size() == 14);
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
		wgsl.stages.push_back(HShader::StageData{.stage = 0, .entryName = "vertexMain", .codeOffset = 0, .codeSize = 1});
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

	SECTION("Stage metadata beyond declared totalSize returns nullopt")
	{
		HShader shader;
		shader.header.magic = HSHADER_MAGIC;
		shader.header.version = HSHADER_VERSION;

		HShader::BackendData wgsl;
		wgsl.stages.push_back(HShader::StageData{.stage = 0, .entryName = "vertexMain", .codeOffset = 0, .codeSize = 2});
		wgsl.bytecode = {std::byte{'v'}, std::byte{'s'}};

		shader.backends.push_back({.backendType = EShaderBackend::WebGPU_WGSL, .stageCount = 1});
		shader.backendData.push_back(std::move(wgsl));

		std::vector<std::byte> buffer;
		HShader::Write(buffer, shader);

		// Shrink the declared container so the stage metadata / bytecode land past it,
		// while the physical buffer still holds them.
		const auto shrunk = static_cast<uint32_t>(sizeof(HShaderHeader) + sizeof(BackendEntry));
		std::memcpy(buffer.data() + offsetof(HShaderHeader, totalSize), &shrunk, sizeof(shrunk));

		REQUIRE_FALSE(HShader::Read(buffer).has_value());
	}

	SECTION("totalSize smaller than header returns nullopt")
	{
		std::vector<std::byte> buffer(sizeof(HShaderHeader) + 8);
		HShaderHeader hdr;
		hdr.magic = HSHADER_MAGIC;
		hdr.version = HSHADER_VERSION;
		hdr.backendCount = 0;
		hdr.totalSize = 4;
		std::memcpy(buffer.data(), &hdr, sizeof(HShaderHeader));

		REQUIRE_FALSE(HShader::Read(buffer).has_value());
	}
}
