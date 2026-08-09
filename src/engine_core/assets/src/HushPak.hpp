#pragma once

#include "AssetFormat.hpp"
#include <cstdint>
#include <span>
#include <string>
#include <vector>
#include <optional>

namespace Hush
{

	static constexpr uint32_t HUSHPAK_MAGIC = 0x4B415048; // 'HPAK'
	static constexpr uint16_t HUSHPAK_VERSION = 1;

#pragma pack(push, 1)
	struct HushPakHeader
	{
		uint32_t magic = HUSHPAK_MAGIC;
		uint16_t version = HUSHPAK_VERSION;
		uint16_t flags = 0;
		uint32_t entryCount = 0;
		uint32_t _reserved0 = 0;
		uint64_t directoryOffset = 0;
		uint64_t stringTableOffset = 0;
		uint64_t stringTableSize = 0;
		uint64_t dataOffset = 0;
		uint64_t totalSize = 0;
		uint8_t _padding[8] = {};
	};

	struct PakEntry
	{
		uint64_t nameHash = 0; // Fnv1a64 of vpath
		uint32_t nameOffset = 0;
		uint32_t nameLength = 0;
		uint64_t dataOffset = 0;
		uint64_t dataSize = 0;
		EAssetFormat format = EAssetFormat::Unknown;
		ECompressionFormat compression = ECompressionFormat::None;
		uint64_t uncompressedSize = 0;
	};
#pragma pack(pop)

	static_assert(sizeof(HushPakHeader) == 64, "HushPakHeader must be 64 bytes");
	static_assert(sizeof(PakEntry) == 48, "PakEntry must be 48 bytes");

	/// Describes a single input to HushPak::Build.
	struct PakInput
	{
		std::string virtualPath;		 // e.g. "textures/stone.png"
		std::span<const std::byte> data; // complete HAsset/HShader blob
	};

	struct HushPak
	{
		HushPakHeader header;
		std::vector<PakEntry> directory;
		std::string stringTable;		  // contiguous name strings
		std::vector<std::byte> blobStore; // all entry blobs concatenated

		/// Build a HushPak from an unsorted list of inputs.
		/// Sorts by nameHash, writes the binary into out.
		static void Build(std::vector<std::byte> &out, std::span<const PakInput> inputs);

		/// Parse a binary HushPak from a span.
		static std::optional<HushPak> Read(std::span<const std::byte> data);

		/// Find an entry by virtual path hash. Binary search.
		/// Returns nullptr if not found.
		const PakEntry *FindEntry(uint64_t nameHash) const;

		/// Get the data span for a given entry (into the original data span).
		[[nodiscard]]
		std::span<const std::byte> EntryData(const PakEntry &entry, std::span<const std::byte> pakData) const;
	};

} // namespace Hush
