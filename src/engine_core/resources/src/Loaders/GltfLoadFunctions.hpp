#pragma once
#include <fastgltf/core.hpp>
#include <fastgltf/tools.hpp>
#include <fastgltf/types.hpp>
#include <fastgltf/glm_element_traits.hpp>
#include <cstdint>
#include <filesystem>
#include <glm/mat4x4.hpp>
#include <span>
#include <vector>
#include "Result.hpp"
#include "Shared/ImageTexture.hpp"
#include "Shared/MaterialPass.hpp"
#include "Logger.hpp"
// #include "Vulkan/GltfMetallicRoughness.hpp"
#include <magic_enum/magic_enum.hpp>

namespace Hush::GltfLoadFunctions
{

	enum class EError
	{
		None = 0,
		FileNotFound,
		InvalidMeshFile,
		FormatNotSupported,
		TextureNotFound,
		NotImplemented
	};

	fastgltf::Expected<fastgltf::Asset> GetAssetFromFile(const std::filesystem::path &file);

	fastgltf::Expected<fastgltf::Asset> GetAssetFromBinary(const std::span<const std::byte> &data);

	glm::mat4 GetNodeTransform(const fastgltf::Node &node);

	EMaterialPass GetMaterialPassFromFastGltfPass(fastgltf::AlphaMode pass);

	std::span<const std::byte> ExtractImageBuffer(const fastgltf::Image &image, const fastgltf::Asset &asset,
												  fastgltf::MimeType *outMimeType);

	/// @brief Get the byte offset and size of an image stored inside a buffer of a glb file.
	/// @param image The image whose data offset is requested.
	/// @param asset The parsed glTF asset.
	/// @param outOffset On success, the byte offset of the image data within the glb file (the buffer view's
	///                  offset). May be nullptr.
	/// @param outSize On success, the byte length of the image data. May be nullptr.
	/// @return true when the image is stored inside the same glb file (as a buffer view), and @p outOffset/@p outSize
	///         have been written. Returns false when the image is not within the same glb file (e.g. an external URI
	///         or base64-embedded data), in which case the outputs are left untouched.
	bool GetImageBufferOffsetAndSize(const fastgltf::Image &image, const fastgltf::Asset &asset, uint64_t *outOffset,
									 uint64_t *outSize);

	// std::shared_ptr<ImageTexture> TextureFromImageDataSource(const fastgltf::Asset &asset,
	// 														 const fastgltf::Image &image);

	EError SetMaterialTextures(void *outMaterialResources, const fastgltf::Asset &asset,
							   const fastgltf::Material &material, const void *loadedTextures);

	Hush::Result<const std::byte *, EError> GetDataFromBufferSource(const fastgltf::Buffer &buffer);

	// TODO: Make this an EachAttributeByName
	template <class BufferType>
	inline std::vector<BufferType> FindAttributeByName(const fastgltf::Primitive &primitive,
													   const fastgltf::Asset &asset,
													   const std::string_view &attributeName)
	{
		std::vector<BufferType> result;
		const fastgltf::Attribute *attribute = primitive.findAttribute(attributeName);
		if (attribute == primitive.attributes.end())
		{
			return result;
		}
		const fastgltf::Accessor &foundAccessor = asset.accessors[attribute->accessorIndex];
		result.reserve(foundAccessor.count);
		fastgltf::iterateAccessor<BufferType>(asset, foundAccessor, [&result](BufferType val) {
		    result.push_back(val);
		});
		return result;
	}

} // namespace Hush::GltfLoadFunctions
