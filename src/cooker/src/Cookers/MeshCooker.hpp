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
			uint32_t surfaceCount;
			uint32_t materialCount;
			uint32_t textureCount;
			/// @brief Virtual path of the original source asset (e.g. a glb) the textures are sliced from
			char sourcePath[256];
		};

		/// @brief Mirrors GLTFLoader::TextureInfo but carries the owning material's resource id and a
		/// file-relative offset into the source asset, so the runtime can seek + read the texture
		/// bytes directly regardless of the source container format.
		struct MeshTextureInfo
		{
			static constexpr size_t MAX_TEX_NAME = 64;
			uint32_t materialResource;
			uint32_t binding;
			uint64_t offset;
			uint64_t size;
			char name[MAX_TEX_NAME];
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
