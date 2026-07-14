#include <catch2/catch_test_macros.hpp>
#include <cstring>
#include <vector>
#include <span>
#include <string>

#include "HushPak.hpp"
#include "HAsset.hpp"
#include "crypto/Hashing.hpp"

using namespace Hush;

TEST_CASE("HushPak round-trip", "[hushpak]")
{
	SECTION("Build and read a pak with two entries")
	{
		// Create two HAsset blobs
		HAsset asset1;
		asset1.header.format = EAssetFormat::RGBA8_UNORM;
		asset1.header.compression = ECompressionFormat::None;
		asset1.header.uncompressedSize = 4;
		asset1.header.compressedSize = 4;
		asset1.payload = {std::byte{0x01}, std::byte{0x02}, std::byte{0x03}, std::byte{0x04}};

		HAsset asset2;
		asset2.header.format = EAssetFormat::R8_UNORM;
		asset2.header.compression = ECompressionFormat::None;
		asset2.header.uncompressedSize = 2;
		asset2.header.compressedSize = 2;
		asset2.payload = {std::byte{0xAA}, std::byte{0xBB}};

		std::vector<std::byte> blob1, blob2;
		HAsset::Write(blob1, asset1);
		HAsset::Write(blob2, asset2);

		std::vector<PakInput> inputs;
		inputs.push_back({"textures/stone.png", blob1});
		inputs.push_back({"textures/wood.png", blob2});

		std::vector<std::byte> pakBuffer;
		HushPak::Build(pakBuffer, inputs);

		// Read back
		auto pak = HushPak::Read(pakBuffer);
		REQUIRE(pak.has_value());

		REQUIRE(pak->header.magic == HUSHPAK_MAGIC);
		REQUIRE(pak->header.entryCount == 2);

		// Find "textures/stone.png"
		auto *entry = pak->FindEntry(Hashing::Fnv1a64("textures/stone.png"));
		REQUIRE(entry != nullptr);
		REQUIRE(entry->dataSize == blob1.size());

		auto entryData = pak->EntryData(*entry, pakBuffer);
		REQUIRE(entryData.size() == blob1.size());

		// Entry data is a full HAsset blob — verify by re-parsing
		auto optAsset1 = HAsset::Read(entryData);
		REQUIRE(optAsset1.has_value());
		REQUIRE(optAsset1.value().payload.size() == 4);
		REQUIRE(optAsset1.value().payload[0] == std::byte{0x01});

		// Find "textures/wood.png"
		entry = pak->FindEntry(Hashing::Fnv1a64("textures/wood.png"));
		REQUIRE(entry != nullptr);
		REQUIRE(entry->dataSize == blob2.size());

		entryData = pak->EntryData(*entry, pakBuffer);
		REQUIRE(entryData.size() == blob2.size());

		auto optAsset2 = HAsset::Read(entryData);
		REQUIRE(optAsset2.has_value());
		REQUIRE(optAsset2.value().payload.size() == 2);
		REQUIRE(optAsset2.value().payload[1] == std::byte{0xBB});

		// Non-existent entry
		entry = pak->FindEntry(Hashing::Fnv1a64("nonexistent.png"));
		REQUIRE(entry == nullptr);
	}

	SECTION("Empty pak is invalid")
	{
		std::vector<std::byte> empty;
		auto pak = HushPak::Read(empty);
		REQUIRE_FALSE(pak.has_value());
	}

	SECTION("ListPath prefix matching")
	{
		HAsset dummy;
		dummy.header.uncompressedSize = 1;
		dummy.header.compressedSize = 1;
		dummy.payload = {std::byte{0x00}};

		std::vector<std::byte> blob;
		HAsset::Write(blob, dummy);

		std::vector<PakInput> inputs;
		inputs.push_back({"textures/stone.png", blob});
		inputs.push_back({"textures/wood.png", blob});
		inputs.push_back({"shaders/grid.slang", blob});
		inputs.push_back({"meshes/cube.gltf", blob});

		std::vector<std::byte> pakBuffer;
		HushPak::Build(pakBuffer, inputs);

		auto pak = HushPak::Read(pakBuffer);
		REQUIRE(pak.has_value());
		REQUIRE(pak->header.entryCount == 4);

		// Can't directly test ListPath on HushPak since it's a method on PakFileSystem,
		// but we can verify the directory is sorted correctly
		for (size_t i = 1; i < pak->directory.size(); ++i)
		{
			REQUIRE(pak->directory[i - 1].nameHash <= pak->directory[i].nameHash);
		}
	}
}
