/*! \file Image.hpp
	\author Alan Ramirez Herrera
	\date 2026-02-21
	\brief Image component for managing image resources
*/

#pragma once

#include "RHI/IGraphicsTexture.hpp"

#include <reflection/Type.hpp>
#include <serialization/Serialization.hpp>
#include <serialization/Deserialization.hpp>
#include <Hushgen.hpp>

#if __has_include("Image.hushgen.hpp") && !defined(HUSH_HEADER_PARSING)
#include "Image.hushgen.hpp"
#endif

#include "HushBindings.hpp"

namespace Hush
{
	/// @brief ECS component that manages an image resource.
	///
	/// An image is a CPU-side resource that holds raw pixel data and metadata (dimensions, format).
	/// It is used as an intermediate representation when loading textures from disk or other sources.
	///
	/// See @ref TextureComponent for the GPU-side texture resource that is created from an Image.
	struct [[hush::export, hush::reflect]] Image
	{
		HUSH_GENERATED_BODY
	public:
		enum class EGpuLoadBehavior
		{
			/// @brief Automatically destroy this Image instance once the GPU texture is created.
			///            This is the default behavior.
			DestroyWhenLoaded,
			/// @brief Keep this Image instance alive after the GPU texture is created.
			///             This allows the raw data to be retained in memory for
			///             potential future use (e.g. re-uploading, CPU-side processing).
			KeepWhenLoaded,
		};
		Image() = default;

		explicit Image(std::vector<std::byte> rawData, uint32_t width, uint32_t height, uint32_t depth,
					   Graphics::ETextureFormat format)
			: m_textureData(std::move(rawData)),
			  m_width(width),
			  m_height(height),
			  m_depth(depth),
			  m_format(format)
		{
		}

		Image(const Image &) = delete;
		Image &operator=(const Image &) = delete;
		Image(Image &&) = default;
		Image &operator=(Image &&) = default;

		~Image() = default;

		[[nodiscard]]
		std::span<const std::byte> GetTextureData() const
		{
			return m_textureData;
		}

		[[nodiscard]]
		uint32_t GetWidth() const
		{
			return m_width;
		}

		[[nodiscard]]
		uint32_t GetHeight() const
		{
			return m_height;
		}

		[[nodiscard]]
		uint32_t GetDepth() const
		{
			return m_depth;
		}

		[[nodiscard]]
		Graphics::ETextureFormat GetFormat() const
		{
			return m_format;
		}

	private:
		std::vector<std::byte> m_textureData; // Raw pixel data for the image

		uint32_t m_width = 0;
		uint32_t m_height = 0;
		uint32_t m_depth = 0;
		Graphics::ETextureFormat m_format = Graphics::ETextureFormat::RGBA8_SRGB;
	};

} // namespace Hush
