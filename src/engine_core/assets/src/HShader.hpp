#pragma once

#include "AssetFormat.hpp"
#include <cstdint>
#include <span>
#include <string>
#include <vector>
#include <optional>

namespace Hush
{

	static constexpr uint32_t HSHADER_MAGIC = 0x44485348; // 'HSHD'
	static constexpr uint16_t HSHADER_VERSION = 1;

#pragma pack(push, 1)
	struct HShaderHeader
	{
		uint32_t magic = HSHADER_MAGIC;
		uint16_t version = HSHADER_VERSION;
		uint16_t backendCount = 0;
		uint32_t totalSize = 0;
	};

	struct BackendEntry
	{
		EShaderBackend backendType = EShaderBackend::WebGPU_WGSL;
		uint32_t stageCount = 0;
		uint64_t dataOffset = 0;
		uint64_t dataSize = 0;
	};

	struct StageEntry
	{
		uint32_t stage = 0; // EShaderStage
		uint32_t entryNameLen = 0;
		// char entryName[entryNameLen] immediately follows
		uint64_t codeOffset = 0;
		uint64_t codeSize = 0;
	};
#pragma pack(pop)

	static_assert(sizeof(HShaderHeader) == 12, "HShaderHeader must be 12 bytes");
	static_assert(sizeof(BackendEntry) == 24, "BackendEntry must be 24 bytes");
	static_assert(sizeof(StageEntry) == 24, "StageEntry must be 24 bytes");

	struct HShader
	{
		HShaderHeader header;
		std::vector<BackendEntry> backends;

		// Per-backend stage data
		struct BackendData
		{
			std::vector<std::string> entryNames;
			std::vector<std::byte> bytecode;
		};
		std::vector<BackendData> backendData;

		static void Write(std::vector<std::byte> &out, const HShader &shader);
		static std::optional<HShader> Read(std::span<const std::byte> data);

		[[nodiscard]]
		size_t SerializedSize() const;
	};

} // namespace Hush
