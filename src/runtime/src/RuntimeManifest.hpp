/*! \file RuntimeManifest.hpp
	\author Alan Ramirez
	\date 2025-11-21
	\brief Manifest of an exported runtime, describes content and modules
*/

#pragma once

#include "Result.hpp"

#include <cstdint>
#include <filesystem>
#include <string>
#include <string_view>
#include <vector>

namespace Hush
{
	/// One module entry of the runtime manifest.
	struct RuntimeModuleManifest
	{
		/// Display name of the module.
		std::string name;

		/// Loader kind: "native-module", "rust", "dotnet-nativeaot" or
		/// "dotnet-coreclr".
		std::string kind;

		/// Path of the native library, relative to the manifest.
		std::string path;

		/// Managed assembly path, only used by dotnet-coreclr modules.
		std::string assembly;

		/// Runtime config path, only used by dotnet-coreclr modules.
		std::string runtimeConfig;
	};

	/// One system entry of the runtime manifest.
	struct RuntimeSystemManifest
	{
		/// Name of the module that owns the system. When empty, the system
		/// is searched in every loaded module.
		std::string module;

		/// Canonical type name of the system, for example "MyGame.TrafficSystem".
		std::string type;
	};

	/// Cooked manifest that tells the runtime player what to load.
	struct RuntimeManifest
	{
		enum class EError : std::uint8_t
		{
			None = 0,
			FileNotFound,
			ParseError,
			UnsupportedVersion,
		};

		/// Newest manifest version this runtime understands.
		static constexpr std::uint32_t CURRENT_VERSION = 1;

		std::uint32_t formatVersion = 0;

		/// Display name of the game.
		std::string name;

		/// Scene loaded at startup, as a virtual path.
		std::string startupScene;

		/// Path of the content pak file, relative to the manifest.
		std::string content;

		std::vector<RuntimeModuleManifest> modules;
		std::vector<RuntimeSystemManifest> systems;

		/// Loads and parses a manifest file.
		static Result<RuntimeManifest, EError> LoadFromFile(const std::filesystem::path &path);

		/// Parses a manifest from JSON text.
		static Result<RuntimeManifest, EError> Parse(std::string_view json);
	};
} // namespace Hush
