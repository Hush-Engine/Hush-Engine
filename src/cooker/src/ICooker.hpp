#pragma once

#include "IFile.hpp"
#include "HMeta.hpp"
#include "Result.hpp"
#include <span>
#include <vector>
#include <string_view>

namespace Hush
{

	enum class ECookError
	{
		None,
		UnsupportedFormat,
		DecodeFailed,
		CompressFailed,
		InvalidMeta,
		FileNotFound,
		Unknown,
	};

	struct CookContext
	{
		std::string sourceVPath;
	};

	/// Interface for cooking a specific asset type.
	class ICooker
	{
	public:
		virtual ~ICooker() = default;

		/// Which file extensions this cooker handles (e.g. PNG, JPEG).
		[[nodiscard]]
		virtual std::span<const EFileExtension> SupportedExtensions() const = 0;

		/// Whether this cooker can handle the given file info.
		[[nodiscard]]
		virtual bool CanCook(const FileInfo &info) const = 0;

		/// Seed default HMeta for a source file.
		[[nodiscard]]
		virtual HMeta DefaultMeta(EFileExtension ext, std::string_view srcVPath) const = 0;

		/// Cook a single asset. The returned data is the uncompressed payload
		/// that will be wrapped in an HAsset by AssetCooker::CookToBlob.
		/// @param input Raw source file bytes.
		/// @param meta  Cooking instructions.
		/// @param ctx   Context (source path for diagnostics).
		/// @return Uncompressed payload + extra data for the HAsset.
		struct CookResult
		{
			std::vector<std::byte> payload;
			std::vector<std::byte> extra;
			EAssetFormat format;
		};
		virtual Result<CookResult, ECookError> Cook(std::span<const std::byte> input, const HMeta &meta,
													const CookContext &ctx) = 0;
	};

} // namespace Hush
