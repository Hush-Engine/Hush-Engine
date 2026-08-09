#include "HShader.hpp"
#include <cstring>

namespace Hush
{

	void HShader::Write(std::vector<std::byte> &out, const HShader &shader)
	{
		if (shader.backends.size() != shader.backendData.size())
		{
			return;
		}

		// Calculate sizes
		const size_t headerSize = sizeof(HShaderHeader);
		const size_t backendArraySize = shader.backends.size() * sizeof(BackendEntry);

		// Build contiguous region: header + BackendEntry[] + per-backend stage metadata + raw bytecode
		std::vector<std::byte> stageMeta; // temporary
		std::vector<std::byte> codeBlob;

		std::vector<uint64_t> backendDataOffsets(shader.backends.size(), 0);
		std::vector<uint64_t> backendDataSizes(shader.backends.size(), 0);
		std::vector<uint64_t> stageMetaOffsets(shader.backends.size(), 0);

		size_t currentStageMetaOffset = 0;
		size_t currentCodeOffset = 0;

		for (size_t b = 0; b < shader.backends.size(); ++b)
		{
			stageMetaOffsets[b] = currentStageMetaOffset;
			const auto &bd = shader.backendData[b];

			for (const auto &stage : bd.stages)
			{
				StageEntry stageEntry{};
				stageEntry.stage = stage.stage;
				stageEntry.entryNameLen = static_cast<uint32_t>(stage.entryName.size());
				stageEntry.codeOffset = stage.codeOffset; // relative to this backend's data block
				stageEntry.codeSize = stage.codeSize;

				// Write stage entry header
				stageMeta.insert(stageMeta.end(), reinterpret_cast<const std::byte *>(&stageEntry),
								 reinterpret_cast<const std::byte *>(&stageEntry + 1));

				// Write entry name
				stageMeta.insert(
					stageMeta.end(), reinterpret_cast<const std::byte *>(stage.entryName.data()),
					reinterpret_cast<const std::byte *>(stage.entryName.data() + stage.entryName.size()));

				// Pad to 8 bytes after entry name
				const size_t pad = (8 - (sizeof(StageEntry) + stage.entryName.size()) % 8) % 8;
				stageMeta.insert(stageMeta.end(), pad, std::byte{0});

				currentStageMetaOffset += sizeof(StageEntry) + stage.entryName.size() + pad;
			}

			backendDataOffsets[b] = currentCodeOffset;
			backendDataSizes[b] = bd.bytecode.size();

			codeBlob.insert(codeBlob.end(), bd.bytecode.begin(), bd.bytecode.end());
			currentCodeOffset += bd.bytecode.size();
		}

		// Fixed-layout: the BackendEntry array sits right after the header,
		// then stage metadata, then raw bytecode.
		const size_t stageMetaOffset = headerSize + backendArraySize;
		const size_t codeOffset = stageMetaOffset + stageMeta.size();

		HShaderHeader hdr = shader.header;
		hdr.backendCount = static_cast<uint16_t>(shader.backends.size());
		hdr.totalSize = static_cast<uint32_t>(headerSize + backendArraySize + stageMeta.size() + codeBlob.size());

		out.resize(hdr.totalSize);

		std::memcpy(out.data(), &hdr, headerSize);

		// Write BackendEntry array with updated offsets
		std::vector<BackendEntry> outBackends = shader.backends;
		for (size_t b = 0; b < outBackends.size(); ++b)
		{
			outBackends[b].dataOffset = backendDataOffsets[b];
			outBackends[b].dataSize = backendDataSizes[b];
		}
		std::memcpy(out.data() + headerSize, outBackends.data(), backendArraySize);

		// Write stage metadata
		if (!stageMeta.empty())
		{
			std::memcpy(out.data() + stageMetaOffset, stageMeta.data(), stageMeta.size());
		}

		// Write bytecode
		if (!codeBlob.empty())
		{
			std::memcpy(out.data() + codeOffset, codeBlob.data(), codeBlob.size());
		}
	}

	std::optional<HShader> HShader::Read(std::span<const std::byte> data)
	{
		if (data.size() < sizeof(HShaderHeader))
		{
			return std::nullopt;
		}

		HShaderHeader hdr;
		std::memcpy(&hdr, data.data(), sizeof(HShaderHeader));

		if (hdr.magic != HSHADER_MAGIC || hdr.version != HSHADER_VERSION)
		{
			return std::nullopt;
		}

		// Enforce the declared container size: trailing bytes past totalSize are not
		// part of this shader and must not be reachable through internal offsets.
		if (hdr.totalSize < sizeof(HShaderHeader) || data.size() < hdr.totalSize)
		{
			return std::nullopt;
		}
		data = data.first(hdr.totalSize);

		HShader shader;
		shader.header = hdr;

		const size_t headerSize = sizeof(HShaderHeader);
		const size_t backendArraySize = static_cast<size_t>(hdr.backendCount) * sizeof(BackendEntry);

		if (backendArraySize > data.size() - headerSize)
		{
			return std::nullopt;
		}

		shader.backends.resize(hdr.backendCount);
		if (hdr.backendCount > 0)
		{
			std::memcpy(shader.backends.data(), data.data() + headerSize, backendArraySize);
		}

		// For each backend, pick the stage metadata and bytecode
		// The stage metadata for each backend is stored sequentially after the BackendEntry array.
		// The spec has per-backend stage metadata before the raw bytecode.
		// Simplified: we just store the raw data per backend.
		// For now, backends hold their stage metadata inline in the bytecode region.
		shader.backendData.resize(hdr.backendCount);

		// The write format packs: header, BackendEntry[], stage entries, raw bytecode.
		// We need to parse the stage entries for each backend.
		size_t cursor = headerSize + backendArraySize;

		for (uint16_t b = 0; b < hdr.backendCount; ++b)
		{
			auto &bd = shader.backendData[b];
			const auto &be = shader.backends[b];

			// Stage entries for this backend precede the bytecode
			for (uint32_t s = 0; s < be.stageCount; ++s)
			{
				if (cursor + sizeof(StageEntry) > data.size())
				{
					return std::nullopt;
				}

				StageEntry stageEntry;
				std::memcpy(&stageEntry, data.data() + cursor, sizeof(StageEntry));
				cursor += sizeof(StageEntry);

				// Per-stage code ranges must stay within this backend's data block.
				if (stageEntry.codeOffset > be.dataSize || stageEntry.codeSize > be.dataSize - stageEntry.codeOffset)
				{
					return std::nullopt;
				}

				// Read entry name
				if (cursor + stageEntry.entryNameLen > data.size())
				{
					return std::nullopt;
				}

				HShader::StageData stageData;
				stageData.stage = stageEntry.stage;
				stageData.entryName =
					std::string(reinterpret_cast<const char *>(data.data() + cursor), stageEntry.entryNameLen);
				stageData.codeOffset = stageEntry.codeOffset;
				stageData.codeSize = stageEntry.codeSize;
				cursor += stageEntry.entryNameLen;

				bd.stages.push_back(std::move(stageData));

				// Padding
				const size_t pad = (8 - (sizeof(StageEntry) + stageEntry.entryNameLen) % 8) % 8;
				cursor += pad;
			}
		}

		// Now cursor points to raw bytecode start
		if (cursor > data.size())
		{
			return std::nullopt;
		}

		// Copy bytecode for each backend
		size_t codeCursor = cursor;
		for (uint16_t b = 0; b < hdr.backendCount; ++b)
		{
			auto &bd = shader.backendData[b];
			const auto &be = shader.backends[b];
			const size_t dataOffset = static_cast<size_t>(be.dataOffset);
			const size_t dataSize = static_cast<size_t>(be.dataSize);
			if (dataOffset > data.size() - codeCursor || dataSize > data.size() - codeCursor - dataOffset)
			{
				return std::nullopt;
			}
			bd.bytecode.resize(dataSize);
			std::memcpy(bd.bytecode.data(), data.data() + dataOffset + codeCursor, dataSize);
		}

		return shader;
	}

	size_t HShader::SerializedSize() const
	{
		size_t size = sizeof(HShaderHeader) + backends.size() * sizeof(BackendEntry);
		for (const auto &bd : backendData)
		{
			for (const auto &stage : bd.stages)
			{
				size += sizeof(StageEntry) + stage.entryName.size();
				const size_t pad = (8 - (sizeof(StageEntry) + stage.entryName.size()) % 8) % 8;
				size += pad;
			}
			size += bd.bytecode.size();
		}
		return size;
	}

} // namespace Hush
