#pragma once

#include <cstdint>

namespace Hush
{

enum class EAssetFormat : uint32_t
{
	Unknown = 0x00,

	// Uncompressed GPU formats
	RGBA8_UNORM = 0x01,
	BGRA8_UNORM = 0x02,
	R8_UNORM = 0x03,
	RG8_UNORM = 0x04,
	RGBA16_FLOAT = 0x05,
	R32_FLOAT = 0x06,

	// BCn compressed (Phase 5)
	DXT1 = 0x10, // BC1
	DXT3 = 0x11,
	DXT5 = 0x12,
	BC4 = 0x13,
	BC5 = 0x14,
	BC7 = 0x15,

	// Special
	Shader = 0x20,
	Mesh = 0x30, // future
};

enum class ECompressionFormat : uint32_t
{
	None = 0,
	Zstd = 1,
};

enum class EShaderBackend : uint32_t
{
	WebGPU_WGSL = 0,
	Vulkan_SPIRV = 1,
	D3D12_DXIL = 2,
};

} // namespace Hush
