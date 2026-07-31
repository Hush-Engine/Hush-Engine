#pragma once

#include "AssetFormat.hpp"
#include "Result.hpp"
#include "serialization/Serialization.hpp"
#include "serialization/Deserialization.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace Hush
{

	struct HMeta
	{
		static constexpr uint16_t VERSION = 1;

		uint16_t version = VERSION;
		uint32_t id = 0;
		uint64_t sourceHash = 0;
		uint64_t sourceMTime = 0;
		std::string assetType; // "texture" | "shader" | "model" | "unknown"

		EAssetFormat outputFormat = EAssetFormat::Unknown;
		ECompressionFormat compression = ECompressionFormat::Zstd;

		struct TextureSettings
		{
			bool sRGB = true;
			bool generateMipmaps = true;
			std::string gpuCompression = "none"; // none|BC1|BC3|BC5|BC7
			uint32_t maxSize = 0;
		};
		TextureSettings texture;

		struct ShaderSettings
		{
			std::vector<EShaderBackend> backends = {EShaderBackend::WebGPU_WGSL};
			std::vector<std::string> entryPoints;
			std::vector<std::string> defines;
		};
		ShaderSettings shader;

		struct ModelSettings
		{
			bool importMaterials = true;
			bool generateLods = false;
			float scaleFactor = 1.0f;
		};
		ModelSettings model;

		// (De)serialization is hand-written (ToJson/FromJson below) rather than routed through
		// the generic Serializer visitor, so it works cross-platform without reflection codegen
		// and covers the nested texture/shader/model settings.

		// Deserialize from a JSON string using RapidJSON DOM
		static Result<HMeta, Serialization::EDeserializationError> FromJson(std::string_view json);

		// Serialize to JSON string
		Result<std::string, Serialization::ESerializationError> ToJson() const;
	};

} // namespace Hush
