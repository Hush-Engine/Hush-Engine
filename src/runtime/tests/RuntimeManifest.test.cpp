#include "RuntimeManifest.hpp"

#include <catch2/catch_test_macros.hpp>
#include <string>

TEST_CASE("Runtime manifests deserialize modules and systems", "[runtime][manifest]")
{
	constexpr std::string_view json = R"({
		"systems":[{"type":"Game.TrafficSystem","module":"Game"}],
		"content":"Content.hushpak",
		"modules":[{"path":"Game.dll","kind":"rust","name":"Game"}],
		"name":"Traffic Game",
		"startupScene":"res://Main.hscene",
		"formatVersion":1
	})";

	auto result = Hush::RuntimeManifest::Parse(json);
	REQUIRE_FALSE(result.has_error());
	const Hush::RuntimeManifest &manifest = result.value();
	REQUIRE(manifest.formatVersion == 1);
	REQUIRE(manifest.name == "Traffic Game");
	REQUIRE(manifest.startupScene == "res://Main.hscene");
	REQUIRE(manifest.content == "Content.hushpak");
	REQUIRE(manifest.modules.size() == 1);
	REQUIRE(manifest.modules[0].name == "Game");
	REQUIRE(manifest.modules[0].kind == "rust");
	REQUIRE(manifest.modules[0].path == "Game.dll");
	REQUIRE(manifest.systems.size() == 1);
	REQUIRE(manifest.systems[0].module == "Game");
	REQUIRE(manifest.systems[0].type == "Game.TrafficSystem");
}

TEST_CASE("Runtime manifests reject invalid versions and entries", "[runtime][manifest]")
{
	auto missingVersion = Hush::RuntimeManifest::Parse(R"({"modules":[]})");
	REQUIRE(missingVersion.has_error());
	REQUIRE(missingVersion.error() == Hush::RuntimeManifest::EError::ParseError);

	auto unsupportedVersion = Hush::RuntimeManifest::Parse(R"({"formatVersion":2})");
	REQUIRE(unsupportedVersion.has_error());
	REQUIRE(unsupportedVersion.error() == Hush::RuntimeManifest::EError::UnsupportedVersion);

	auto invalidModule = Hush::RuntimeManifest::Parse(R"({"formatVersion":1,"modules":[42]})");
	REQUIRE(invalidModule.has_error());
	REQUIRE(invalidModule.error() == Hush::RuntimeManifest::EError::ParseError);
}

TEST_CASE("Runtime manifests respect bounded string views", "[runtime][manifest]")
{
	const std::string storage = R"({"formatVersion":1}ignored bytes)";
	const std::string_view json(storage.data(), storage.find('}') + 1);

	auto result = Hush::RuntimeManifest::Parse(json);
	REQUIRE_FALSE(result.has_error());
	REQUIRE(result.value().formatVersion == 1);
}
