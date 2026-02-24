/*! \file IGraphicsTexture.hpp
	\author Alan Ramirez Herrera
	\date 2026-02-17
	\brief Abstract texture interface for graphics abstraction
*/
#pragma once

#include "GraphicsTypes.hpp"

namespace Hush::Graphics
{
	/// @brief Abstract texture interface
	class IGraphicsTexture
	{
	public:
		IGraphicsTexture() = default;
		virtual ~IGraphicsTexture() = default;

		IGraphicsTexture(const IGraphicsTexture &) = delete;
		IGraphicsTexture &operator=(const IGraphicsTexture &) = delete;
		IGraphicsTexture(IGraphicsTexture &&) = delete;
		IGraphicsTexture &operator=(IGraphicsTexture &&) = delete;

		/// @brief Get texture width
		[[nodiscard]]
		virtual uint32_t GetWidth() const = 0;

		/// @brief Get texture height
		[[nodiscard]]
		virtual uint32_t GetHeight() const = 0;

		/// @brief Get texture depth
		[[nodiscard]]
		virtual uint32_t GetDepth() const = 0;

		/// @brief Get texture format
		[[nodiscard]]
		virtual ETextureFormat GetFormat() const = 0;

		/// @brief Get mip level count
		[[nodiscard]]
		virtual uint32_t GetMipLevels() const = 0;

		/// @brief Get native handle (API-specific)
		[[nodiscard]]
		virtual void *GetNativeHandle() const = 0;
	};

} // namespace Hush::Graphics
