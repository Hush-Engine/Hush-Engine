/*! \file IGraphicsBuffer.hpp
	\author Alan Ramirez Herrera
	\date 2025-01-17
	\brief Abstract buffer interface for graphics abstraction
*/
#pragma once

#include "GraphicsTypes.hpp"

namespace Hush::Graphics
{
	/// @brief Abstract buffer interface
	class IGraphicsBuffer
	{
	public:
		IGraphicsBuffer() = default;
		virtual ~IGraphicsBuffer() = default;

		IGraphicsBuffer(const IGraphicsBuffer &) = delete;
		IGraphicsBuffer &operator=(const IGraphicsBuffer &) = delete;
		IGraphicsBuffer(IGraphicsBuffer &&) = delete;
		IGraphicsBuffer &operator=(IGraphicsBuffer &&) = delete;

		/// @brief Get buffer size in bytes
		[[nodiscard]]
		virtual uint64_t GetSize() const = 0;

		/// @brief Get buffer usage flags
		[[nodiscard]]
		virtual EBufferUsage GetUsage() const = 0;

		/// @brief Map buffer for CPU access (if supported)
		/// @return Pointer to mapped memory, or nullptr if mapping failed
		virtual void *Map() = 0;

		/// @brief Unmap buffer
		virtual void Unmap() = 0;

		/// @brief Get native handle (API-specific)
		[[nodiscard]]
		virtual void *GetNativeHandle() const = 0;
	};

} // namespace Hush::Graphics
