#include "HushPak.hpp"
#include "HAsset.hpp"
#include "crypto/Hashing.hpp"
#include <algorithm>
#include <cstring>

namespace Hush
{

void HushPak::Build(std::vector<std::byte> &out, std::span<const PakInput> inputs)
{
	// Build directory entries
	std::vector<PakEntry> entries;
	entries.reserve(inputs.size());

	size_t stringTableSize = 0;
	size_t totalDataSize = 0;

	for (const auto &input : inputs)
	{
		PakEntry entry{};
		entry.nameHash = Hashing::Fnv1a64(input.virtualPath);
		entry.nameLength = static_cast<uint32_t>(input.virtualPath.size());
		entry.dataSize = input.data.size();
		entry.uncompressedSize = input.data.size();

		// We don't know format/compression yet — they're embedded in the HAsset/HShader blob.
		entry.format = EAssetFormat::Unknown;
		entry.compression = ECompressionFormat::None;

		entries.push_back(entry);
		stringTableSize += input.virtualPath.size();
		totalDataSize += input.data.size();
	}

	// Sort by nameHash for binary search
	std::sort(entries.begin(), entries.end(),
			  [](const PakEntry &a, const PakEntry &b) { return a.nameHash < b.nameHash; });

	// Calculate offsets
	const size_t headerSize = sizeof(HushPakHeader);
	const size_t directorySize = entries.size() * sizeof(PakEntry);
	const size_t dirOffset = headerSize;
	const size_t strOffset = dirOffset + directorySize;
	const size_t dataOffset = strOffset + stringTableSize;

	HushPakHeader hdr;
	hdr.magic = HUSHPAK_MAGIC;
	hdr.version = HUSHPAK_VERSION;
	hdr.flags = 0;
	hdr.entryCount = static_cast<uint32_t>(entries.size());
	hdr.directoryOffset = dirOffset;
	hdr.stringTableOffset = strOffset;
	hdr.stringTableSize = stringTableSize;
	hdr.dataOffset = dataOffset;
	hdr.totalSize = dataOffset + totalDataSize;

	out.resize(static_cast<size_t>(hdr.totalSize));

	// Write header
	std::memcpy(out.data(), &hdr, headerSize);

	// Write directory
	std::memcpy(out.data() + dirOffset, entries.data(), directorySize);

	// Write string table and data — entries are sorted, but we need to iterate
	// in the same sorted order to assign offsets correctly.
	size_t strCursor = 0;
	size_t dataCursor = 0;

	// We need to pair sorted entries back to their inputs.
	// Build a reverse mapping from sorted index to input index.
	// Since inputs is unordered, we need to match entries back by nameHash.
	// Simpler: just re-derive the layout by iterating sorted entries,
	// storing the blob data for each.
	for (size_t i = 0; i < entries.size(); ++i)
	{
		// Find the matching input
		for (const auto &input : inputs)
		{
			if (Hashing::Fnv1a64(input.virtualPath) == entries[i].nameHash)
			{
				entries[i].nameOffset = static_cast<uint32_t>(strCursor);
				std::memcpy(out.data() + strOffset + strCursor, input.virtualPath.data(), input.virtualPath.size());
				strCursor += input.virtualPath.size();

				entries[i].dataOffset = dataCursor;
				std::memcpy(out.data() + dataOffset + dataCursor, input.data.data(), input.data.size());
				dataCursor += input.data.size();

				// Sniff format from the blob header
				if (input.data.size() >= sizeof(uint32_t))
				{
					uint32_t magic;
					std::memcpy(&magic, input.data.data(), sizeof(uint32_t));
					if (magic == HASSET_MAGIC)
					{
						// Read format from the HAsset header inside the blob
						if (input.data.size() >= offsetof(HAssetHeader, format) + sizeof(EAssetFormat))
						{
							auto *hdrPtr = reinterpret_cast<const HAssetHeader *>(input.data.data());
							entries[i].format = hdrPtr->format;
							entries[i].compression = hdrPtr->compression;
							entries[i].uncompressedSize = hdrPtr->uncompressedSize;
						}
					}
				}
				break;
			}
		}
	}

	// Re-write directory with updated offsets
	std::memcpy(out.data() + dirOffset, entries.data(), directorySize);
}

std::optional<HushPak> HushPak::Read(std::span<const std::byte> data)
{
	if (data.size() < sizeof(HushPakHeader))
	{
		return std::nullopt;
	}

	HushPakHeader hdr;
	std::memcpy(&hdr, data.data(), sizeof(HushPakHeader));

	if (hdr.magic != HUSHPAK_MAGIC || hdr.version != HUSHPAK_VERSION)
	{
		return std::nullopt;
	}

	if (data.size() < hdr.totalSize)
	{
		return std::nullopt;
	}

	HushPak pak;
	pak.header = hdr;

	// Read directory
	const size_t dirCount = hdr.entryCount;
	pak.directory.resize(dirCount);
	if (dirCount > 0)
	{
		std::memcpy(pak.directory.data(), data.data() + hdr.directoryOffset, dirCount * sizeof(PakEntry));

		// Read string table
		pak.stringTable.resize(hdr.stringTableSize);
		if (hdr.stringTableSize > 0)
		{
			std::memcpy(pak.stringTable.data(), data.data() + hdr.stringTableOffset, hdr.stringTableSize);
		}
	}

	// blobStore references the data region; caller should keep the original data alive.
	// For simplicity we keep a reference; the data slice is in the caller's span.
	// We don't copy blob data here — use EntryData() instead.
	return pak;
}

const PakEntry *HushPak::FindEntry(uint64_t nameHash) const
{
	auto it = std::lower_bound(directory.begin(), directory.end(), nameHash,
							   [](const PakEntry &entry, uint64_t hash) { return entry.nameHash < hash; });

	if (it != directory.end() && it->nameHash == nameHash)
	{
		return &(*it);
	}
	return nullptr;
}

std::span<const std::byte> HushPak::EntryData(const PakEntry &entry, std::span<const std::byte> pakData) const
{
	const size_t dataStart = static_cast<size_t>(header.dataOffset) + static_cast<size_t>(entry.dataOffset);
	if (dataStart + entry.dataSize > pakData.size())
	{
		return {};
	}
	return pakData.subspan(dataStart, static_cast<size_t>(entry.dataSize));
}

} // namespace Hush
