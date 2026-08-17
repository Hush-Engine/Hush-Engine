#pragma once

#include "ICooker.hpp"
#include <array>
#include <cstdint>

namespace Hush
{
	class MeshCooker final : public ICooker
	{

		struct Header
		{
			uint32_t vertexCount;
			uint32_t indexCount;
			uint32_t materialCount;
		};

		static constexpr std::array<EFileExtension, 2> EXTENSIONS = {EFileExtension::GLB, EFileExtension::GLTF};

		[[nodiscard]]
		std::span<const EFileExtension> SupportedExtensions() const override;

		/// Whether this cooker can handle the given file info.
		[[nodiscard]]
		bool CanCook(const FileInfo &info) const override;

		/// Seed default HMeta for a source file.
		[[nodiscard]]
		HMeta DefaultMeta(EFileExtension ext, std::string_view srcVPath) const override;

		Result<CookResult, ECookError> Cook(std::span<const std::byte> input, const HMeta &meta,
											const CookContext &ctx) override;
	};

} // namespace Hush
