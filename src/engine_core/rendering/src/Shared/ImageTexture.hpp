/*! \file ImageTexture.hpp
	\author Kyn21kx
	\date 2025-01-03
	\brief Defines the properties of a texture as decoded data
*/

#pragma once
#include <span>
#include <cstddef>
#include <filesystem>

namespace Hush
{
	class ImageTexture
	{

	public:
		ImageTexture() = default;

		ImageTexture(const std::filesystem::path &filePath);

		ImageTexture(const std::byte *data, size_t size);

		[[nodiscard]] const int32_t &GetWidth() const noexcept;

		[[nodiscard]] const int32_t &GetHeight() const noexcept;

		[[nodiscard]] const std::byte *GetImageData() const;

	private:
		int32_t m_width{};
		int32_t m_height{};
		std::unique_ptr<std::byte> m_data;
	};
} // namespace Hush
