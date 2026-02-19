/*! \file IGraphicsDevice.hpp
	\author Alan Ramirez Herrera
	\date 2025-01-17
	\brief Graphics device interface for cross-API abstraction
*/
#pragma once

#include "GraphicsTypes.hpp"
#include "IGraphicsBuffer.hpp"
#include "IGraphicsTexture.hpp"
#include "ICommandQueue.hpp"
#include "ICommandList.hpp"
#include <memory>
#include <functional>

namespace Hush::Graphics
{
	// ============================================================================
	// Graphics Device Interface
	// ============================================================================

	/// @brief Abstract graphics device interface
	/// Provides a unified API for creating resources and submitting commands
	class IGraphicsDevice
	{
	public:
		IGraphicsDevice() = default;
		virtual ~IGraphicsDevice() = default;

		IGraphicsDevice(const IGraphicsDevice&) = delete;
		IGraphicsDevice& operator=(const IGraphicsDevice&) = delete;
		IGraphicsDevice(IGraphicsDevice&&) = delete;
		IGraphicsDevice& operator=(IGraphicsDevice&&) = delete;

		// ========================================================================
		// Device Information
		// ========================================================================

		/// @brief Get the graphics API backend
		[[nodiscard]] virtual EGraphicsAPI GetAPI() const = 0;

		/// @brief Get device capabilities
		[[nodiscard]] virtual GraphicsDeviceCapabilities GetCapabilities() const = 0;

		/// @brief Check if device is initialized
		[[nodiscard]] virtual bool IsInitialized() const = 0;

		// ========================================================================
		// Resource Creation
		// ========================================================================

		/// @brief Create a buffer
		/// @param descriptor Buffer creation parameters
		/// @return Created buffer, or nullptr on failure
		[[nodiscard]] virtual std::shared_ptr<IGraphicsBuffer> CreateBuffer(
			const BufferDescriptor& descriptor) = 0;

		/// @brief Create a texture
		/// @param descriptor Texture creation parameters
		/// @return Created texture, or nullptr on failure
		[[nodiscard]] virtual std::unique_ptr<IGraphicsTexture> CreateTexture(
			const TextureDescriptor& descriptor) = 0;

		// ========================================================================
		// Command List Creation
		// ========================================================================

		/// @brief Create a copy command list
		/// @return Created command list, or nullptr on failure
		[[nodiscard]] virtual std::unique_ptr<ICopyCommandList> CreateCopyCommandList() = 0;

		/// @brief Create a compute command list
		/// @return Created command list, or nullptr on failure
		[[nodiscard]] virtual std::unique_ptr<IComputeCommandList> CreateComputeCommandList() = 0;

		/// @brief Create a graphics command list
		/// @return Created command list, or nullptr on failure
		[[nodiscard]] virtual std::unique_ptr<IGraphicsCommandList> CreateGraphicsCommandList() = 0;

		/// @brief Get the main graphics queue
		/// @note This might return the same queue as GetComputeQueue or GetTransferQueue if the API doesn't support separate queues
		///       Which is the case for WebGPU. To check if separate queues are supported, use the capabilities struct.
		[[nodiscard]] virtual ICommandQueue* GetGraphicsQueue() = 0;

		/// @brief Get a compute queue (if supported)
		[[nodiscard]] virtual ICommandQueue* GetComputeQueue() = 0;

		/// @brief Get a transfer queue (if supported)
		[[nodiscard]] virtual ICommandQueue* GetTransferQueue() = 0;

		// ========================================================================
		// Frame Management
		// ========================================================================

		/// @brief Begin a new frame
		/// @return Swapchain texture handle, or nullptr on failure
		virtual void BeginFrame() = 0;

		/// @brief End frame and present
		virtual void EndFrame() = 0;

		/// @brief Get the current frame's swapchain texture
		[[nodiscard]]
		virtual IGraphicsTexture* GetCurrentFrameTexture() const = 0;

		/// @brief Resize the swapchain
		/// @param width New width
		/// @param height New height
		virtual void Resize(uint32_t width, uint32_t height) = 0;

		/// @brief Add a function to the deletion queue for deferred cleanup
		/// @param deleteFunc Deletion function
		virtual void AddToDeletionQueue(std::function<void()>&& deleteFunc) = 0;

		/// @brief Flush deletion queue
		virtual void FlushDeletionQueue() = 0;

		/// @brief Get native device handle (API-specific)
		[[nodiscard]] virtual void* GetNativeHandle() const = 0;
	};

} // namespace Hush::Graphics
