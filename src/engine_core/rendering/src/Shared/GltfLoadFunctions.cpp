#include "GltfLoadFunctions.hpp"
#include "Assertions.hpp"
#include "Result.hpp"
#include "Shared/ImageTexture.hpp"
#include <cstdint>
#include <fastgltf/types.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/quaternion.hpp>
#include "MaterialPass.hpp"


glm::mat4 Hush::GltfLoadFunctions::GetNodeTransform(const fastgltf::Node& node)
{
    const fastgltf::math::mat<float, 4, 4> *const matrix = std::get_if<fastgltf::math::fmat4x4>(&node.transform);
	if (matrix != nullptr) {
		// Matrix is column-major, glm expects column-major
		return *reinterpret_cast<const glm::mat4*>(matrix->data());
	}
	const fastgltf::TRS* trsMatrix = std::get_if<fastgltf::TRS>(&node.transform);
	if (trsMatrix == nullptr) {
		return {1.0F};
	}
	// Use TRS components
	glm::vec3 translation = *reinterpret_cast<const glm::vec3*>(trsMatrix->translation.data());

	glm::quat rotation = *reinterpret_cast<const glm::quat*>(trsMatrix->rotation.value_ptr());

	glm::vec3 scale = *reinterpret_cast<const glm::vec3*>(trsMatrix->scale.data());

	return glm::translate(glm::mat4(1.0F), translation) *
		glm::mat4_cast(rotation) *
		glm::scale(glm::mat4(1.0F), scale);
	
}

Hush::EMaterialPass Hush::GltfLoadFunctions::GetMaterialPassFromFastGltfPass(fastgltf::AlphaMode pass)
{
	switch (pass)
	{
	case fastgltf::AlphaMode::Opaque:
		return EMaterialPass::MainColor;
	case fastgltf::AlphaMode::Blend:
		return EMaterialPass::Transparent;
	case fastgltf::AlphaMode::Mask:
		return EMaterialPass::Mask;
	default:
		return EMaterialPass::Other;
	}
}

std::shared_ptr<Hush::ImageTexture> Hush::GltfLoadFunctions::TextureFromImageDataSource(const fastgltf::Asset& asset, const fastgltf::Image& image)
{
	const fastgltf::sources::Vector* vectorData = std::get_if<fastgltf::sources::Vector>(&image.data);
	if (vectorData != nullptr) {
		return std::make_shared<ImageTexture>(reinterpret_cast<const std::byte*>(vectorData->bytes.data()), vectorData->bytes.size());
	}
	const fastgltf::sources::URI* uriData = std::get_if<fastgltf::sources::URI>(&image.data);
	const fastgltf::sources::BufferView* bufferViewData = std::get_if<fastgltf::sources::BufferView>(&image.data);

	// Buffer view index
	if (bufferViewData != nullptr) {
		const fastgltf::BufferView& bufferView = asset.bufferViews.at(bufferViewData->bufferViewIndex);
		const fastgltf::Buffer& buffer = asset.buffers.at(bufferView.bufferIndex);
		Result<const std::byte*, EError> bufferData = GetDataFromBufferSource(buffer);
		if (bufferData.has_error()) {
			HUSH_ASSERT(false, "Unrecognized data format!!");
		}
		
		return std::make_shared<ImageTexture>(bufferData.value() + bufferView.byteOffset, bufferView.byteLength);
	}

	// TODO: support for file byte offset
	if (uriData == nullptr || uriData->fileByteOffset > 0) {
		return nullptr;
	}
	return std::make_shared<ImageTexture>(uriData->uri.fspath());
}

Hush::Result<const std::byte*, Hush::GltfLoadFunctions::EError> Hush::GltfLoadFunctions::GetDataFromBufferSource(const fastgltf::Buffer& buffer)
{
	const fastgltf::sources::Vector* vectorData = std::get_if<fastgltf::sources::Vector>(&buffer.data);
	if (vectorData != nullptr) {
		return vectorData->bytes.data();
	}
	
	const fastgltf::sources::Array* arrayData = std::get_if<fastgltf::sources::Array>(&buffer.data);
	
	if (arrayData != nullptr) {
		return arrayData->bytes.data();
	}
	//Ok, try the ByteView
	const fastgltf::sources::ByteView* byteData = std::get_if<fastgltf::sources::ByteView>(&buffer.data);
	if (byteData != nullptr) {
		return byteData->bytes.data();
	}
	
	//Else, idk, we don't recognize this yet
	return EError::InvalidMeshFile;
}
