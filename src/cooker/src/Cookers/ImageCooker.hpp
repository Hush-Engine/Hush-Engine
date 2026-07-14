#pragma once

#include "ICooker.hpp"
#include <array>

namespace Hush
{

/// Cooks PNG/JPEG source images into RGBA8 HAsset blobs.
/// BCn compression and mipmap generation deferred to Phase 5.
class ImageCooker final : public ICooker
{
public:
	static constexpr std::array<EFileExtension, 2> EXTENSIONS = {EFileExtension::PNG, EFileExtension::JPEG};

	std::span<const EFileExtension> SupportedExtensions() const override;
	bool CanCook(const FileInfo &info) const override;
	HMeta DefaultMeta(EFileExtension ext, std::string_view srcVPath) const override;
	Result<CookResult, ECookError> Cook(std::span<const std::byte> input, const HMeta &meta,
										const CookContext &ctx) override;
};

} // namespace Hush
