/*! \file GraphicsTypes.hpp
	\author Alan Ramirez Herrera
	\date 2025-01-17
	\brief Common types, enumerations, and structures for graphics abstraction
*/
#pragma once

#include <cstdint>

namespace Hush::Graphics
{
	class IFrameBuffer
	{
	public:
		virtual ~IFrameBuffer() = default;

		IFrameBuffer(const IFrameBuffer &) = delete;
		IFrameBuffer(IFrameBuffer &&) = delete;
		IFrameBuffer &operator=(const IFrameBuffer &) = delete;
		IFrameBuffer &operator=(IFrameBuffer &&) = delete;

		[[nodiscard]]
		virtual uint32_t GetWidth() const = 0;
		[[nodiscard]]
		virtual uint32_t GetHeight() const = 0;
	};
} // namespace Hush::Graphics
