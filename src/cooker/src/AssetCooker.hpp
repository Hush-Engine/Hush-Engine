#pragma once

#include "ICooker.hpp"
#include "HAsset.hpp"
#include "HushPak.hpp"
#include "Result.hpp"
#include <memory>
#include <vector>
#include <unordered_map>
#include <filesystem>

namespace Hush
{

/// Central entry point for cooking assets.
/// Shared between the CLI and the editor (via CookerService).
class AssetCooker
{
public:
	AssetCooker();
	~AssetCooker();

	AssetCooker(const AssetCooker &) = delete;
	AssetCooker &operator=(const AssetCooker &) = delete;

	// Movable so it can be held by value in a relocatable ECS component (flecs may
	// move components when an entity's archetype changes).
	AssetCooker(AssetCooker &&) noexcept;
	AssetCooker &operator=(AssetCooker &&) noexcept;

	/// Register built-in cookers (Image, Shader).
	void RegisterBuiltins();

	/// Register a custom cooker.
	void RegisterCooker(std::unique_ptr<ICooker> cooker);

	/// Find a cooker for a given source extension.
	[[nodiscard]]
	ICooker *FindCooker(EFileExtension ext) const;

	/// Convert a source path extension to an EFileExtension value.
	[[nodiscard]]
	static EFileExtension ExtensionFromPath(const std::filesystem::path &path);

	/// Cook a source blob to a complete HAsset (compressed if HMeta says so).
	/// @return Complete HAsset binary blob, or error.
	Result<std::vector<std::byte>, ECookError> CookToBlob(std::span<const std::byte> input,
												  EFileExtension sourceExtension,
												  const HMeta &meta,
												  const CookContext &ctx);

	/// Walk a content directory: for each source file, refresh .hmeta if needed,
	/// cook to .hcooked/{id}.hasset.
	Result<void, ECookError> CookDirectory(const std::filesystem::path &contentDir,
										   ECompressionFormat compression = ECompressionFormat::Zstd);

	/// Build a .hushpak from a set of cooked blobs.
	Result<void, ECookError> BuildPak(std::span<const PakInput> inputs, IFile &out);

private:
	std::vector<std::unique_ptr<ICooker>> m_cookers;
};

} // namespace Hush
