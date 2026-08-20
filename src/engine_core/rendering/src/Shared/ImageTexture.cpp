/*! \file ImageTexture.cpp
	\author Kyn21kx
	\date 2025-01-03
	\brief
*/

#include "ImageTexture.hpp"
#include <cstddef>
#include "Assertions.hpp"
// NOTE: STB_IMAGE_IMPLEMENTATION lives in engine_core/src/stb_impl.cpp (single TU per
// final link). Do not define it here; this file only consumes the STB API.
#include <stb/stb_image.h>

Hush::ImageTexture::ImageTexture(const std::filesystem::path &filePath)
{
	HUSH_ASSERT(std::filesystem::exists(filePath), "Image texture path is not valid!");
	int32_t componentCount = 0;
	void *rawData = stbi_load(filePath.string().c_str(), &this->m_width, &this->m_height, &componentCount, 0);
	this->m_data = reinterpret_cast<std::byte *>(rawData);
	(void)componentCount;
}

Hush::ImageTexture::ImageTexture(const std::byte *data, size_t size)
{
	HUSH_ASSERT(data != nullptr, "Invalid texture data (nullptr)!");
	int32_t componentCount = 0;
	uint8_t *rawData = stbi_load_from_memory(reinterpret_cast<const uint8_t *>(data), static_cast<int32_t>(size),
											 &this->m_width, &this->m_height, &componentCount, 4);
	this->m_data = reinterpret_cast<std::byte *>(rawData);
	(void)componentCount;
}

Hush::ImageTexture::~ImageTexture()
{
	stbi_image_free(this->m_data);
}

int32_t Hush::ImageTexture::GetWidth() const noexcept
{
	return this->m_width;
}

int32_t Hush::ImageTexture::GetHeight() const noexcept
{
	return this->m_height;
}

const std::byte *Hush::ImageTexture::GetImageData() const noexcept
{
	return this->m_data;
}
