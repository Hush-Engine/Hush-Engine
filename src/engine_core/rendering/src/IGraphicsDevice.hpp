/*! \file IGraphicsDevice.hpp
	\author Alan Ramirez Herrera
	\date 2025-01-17
	\brief Generic graphics device interface for cross-API abstraction
*/
#pragma once

#include <cstdint>
#include <memory>
#include <string_view>
#include <functional>

namespace Hush::Graphics
{
	// ============================================================================
	// Forward Declarations
	// ============================================================================

	class ICommandQueue;
	class IGraphicsBuffer;
	class IGraphicsTexture;
	class IShaderModule;
	class IPipeline;
	class IFramebuffer;

	// ============================================================================
	// Enumerations
	// ============================================================================

	/// @brief Graphics API backend type
	enum class EGraphicsAPI
	{
		Vulkan,
		D3D12,
		Metal,
		OpenGL,
		WebGPU,
	};

	/// @brief Buffer usage flags
	enum class EBufferUsage : uint32_t
	{
		None = 0,
		Vertex = 1 << 0,
		Index = 1 << 1,
		Uniform = 1 << 2,
		Storage = 1 << 3,
		CopySource = 1 << 4,
		CopyDestination = 1 << 5,
		Indirect = 1 << 6,
	};

	inline EBufferUsage operator|(EBufferUsage a, EBufferUsage b)
	{
		return static_cast<EBufferUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	inline EBufferUsage operator&(EBufferUsage a, EBufferUsage b)
	{
		return static_cast<EBufferUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}

	/// @brief Texture usage flags
	enum class ETextureUsage : uint32_t
	{
		None = 0,
		Sampled = 1 << 0,
		Storage = 1 << 1,
		RenderTarget = 1 << 2,
		DepthStencil = 1 << 3,
		CopySource = 1 << 4,
		CopyDestination = 1 << 5,
	};

	inline ETextureUsage operator|(ETextureUsage a, ETextureUsage b)
	{
		return static_cast<ETextureUsage>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	inline ETextureUsage operator&(ETextureUsage a, ETextureUsage b)
	{
		return static_cast<ETextureUsage>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}

	// NOLINTBEGIN(readability-identifier-naming)
	/// @brief Texture format
	enum class ETextureFormat
	{
		// 8-bit formats
		R8_UNORM,
		R8_SNORM,
		R8_UINT,
		R8_SINT,

		// 16-bit formats
		R16_UNORM,
		R16_SNORM,
		R16_UINT,
		R16_SINT,
		R16_FLOAT,

		// 32-bit formats
		R32_UINT,
		R32_SINT,
		R32_FLOAT,

		// RG formats
		RG8_UNORM,
		RG8_SNORM,
		RG16_FLOAT,
		RG32_FLOAT,

		// RGB formats
		RGB8_UNORM,
		RGB8_SRGB,

		// RGBA formats
		RGBA8_UNORM,
		RGBA8_SRGB,
		RGBA16_FLOAT,
		RGBA32_FLOAT,

		// BGRA formats
		BGRA8_UNORM,
		BGRA8_SRGB,

		// Depth formats
		D16_UNORM,
		D24_UNORM,
		D32_FLOAT,
		D24_UNORM_S8_UINT,
		D32_FLOAT_S8_UINT,

		// Compressed formats
		BC1_UNORM,
		BC1_SRGB,
		BC3_UNORM,
		BC3_SRGB,
		BC4_UNORM,
		BC5_UNORM,
		BC7_UNORM,
		BC7_SRGB,
	};
	// NOLINTEND(readability-identifier-naming)

	/// @brief Memory access flags
	enum class EMemoryAccess
	{
		CPUNone,      // GPU only
		CPUWrite,     // CPU can write, GPU can read
		CPURead,      // CPU can read, GPU can write
		CPUReadWrite, // CPU can read/write
	};

	/// @brief Queue type for command submission
	enum class EQueueType
	{
		Graphics,
		Compute,
		Transfer,
	};

	// ============================================================================
	// Descriptor Structures
	// ============================================================================

	/// @brief Buffer creation descriptor
	struct BufferDescriptor
	{
		uint64_t size = 0;
		EBufferUsage usage = EBufferUsage::None;
		EMemoryAccess memoryAccess = EMemoryAccess::CPUNone;
		const char* debugName = nullptr;
	};

	/// @brief Texture creation descriptor
	struct TextureDescriptor
	{
		uint32_t width = 1;
		uint32_t height = 1;
		uint32_t depth = 1;
		uint32_t mipLevels = 1;
		uint32_t arrayLayers = 1;
		uint32_t sampleCount = 1;
		ETextureFormat format = ETextureFormat::RGBA8_UNORM;
		ETextureUsage usage = ETextureUsage::Sampled;
		const char* debugName = nullptr;
	};

	/// @brief Device capabilities
	struct GraphicsDeviceCapabilities
	{
		uint64_t maxBufferSize = 0;
		uint32_t maxTextureDimension2D = 0;
		uint32_t maxTextureDimension3D = 0;
		uint32_t maxTextureArrayLayers = 0;
		uint64_t maxUniformBufferBindingSize = 0;
		uint64_t maxStorageBufferBindingSize = 0;
		uint32_t maxColorAttachments = 0;
		bool supportsCompute = false;
		bool supportsGeometryShader = false;
		bool supportsTessellation = false;
		bool supportsRayTracing = false;
	};

	// ============================================================================
	// Resource Interfaces
	// ============================================================================]\

	/// @brief Abstract buffer interface
	class IGraphicsBuffer
	{
	public:
        IGraphicsBuffer() = default;
		virtual ~IGraphicsBuffer() = default;

		IGraphicsBuffer(const IGraphicsBuffer&) = delete;
		IGraphicsBuffer& operator=(const IGraphicsBuffer&) = delete;
        IGraphicsBuffer(IGraphicsBuffer&&) = delete;
        IGraphicsBuffer& operator=(IGraphicsBuffer&&) = delete;

		/// @brief Get buffer size in bytes
		[[nodiscard]] virtual uint64_t GetSize() const = 0;

		/// @brief Get buffer usage flags
		[[nodiscard]] virtual EBufferUsage GetUsage() const = 0;

		/// @brief Map buffer for CPU access (if supported)
		/// @return Pointer to mapped memory, or nullptr if mapping failed
		virtual void* Map() = 0;

		/// @brief Unmap buffer
		virtual void Unmap() = 0;

		/// @brief Get native handle (API-specific)
		[[nodiscard]] virtual void* GetNativeHandle() const = 0;
	};

	/// @brief Abstract texture interface
	class IGraphicsTexture
	{
	public:
        IGraphicsTexture() = default;
		virtual ~IGraphicsTexture() = default;

		IGraphicsTexture(const IGraphicsTexture&) = delete;
		IGraphicsTexture& operator=(const IGraphicsTexture&) = delete;
        IGraphicsTexture(IGraphicsTexture&&) = delete;
        IGraphicsTexture& operator=(IGraphicsTexture&&) = delete;

		/// @brief Get texture width
		[[nodiscard]] virtual uint32_t GetWidth() const = 0;

		/// @brief Get texture height
		[[nodiscard]] virtual uint32_t GetHeight() const = 0;

		/// @brief Get texture depth
		[[nodiscard]] virtual uint32_t GetDepth() const = 0;

		/// @brief Get texture format
		[[nodiscard]] virtual ETextureFormat GetFormat() const = 0;

		/// @brief Get mip level count
		[[nodiscard]] virtual uint32_t GetMipLevels() const = 0;

		/// @brief Get native handle (API-specific)
		[[nodiscard]] virtual void* GetNativeHandle() const = 0;
	};

	// ============================================================================
	// Command Queue Interface
	// ============================================================================


	class ICommandList
	{
	public:
        ICommandList() = default;
        virtual ~ICommandList() = default;

        ICommandList(const ICommandList&) = delete;
        ICommandList& operator=(const ICommandList&) = delete;
        ICommandList(ICommandList&&) = delete;
        ICommandList& operator=(ICommandList&&) = delete;

        /// @brief Reset command list for recording
        virtual void Reset() = 0;

        /// @brief Close command list after recording
        virtual void Close() = 0;

        /// @brief Get native handle (API-specific)
        [[nodiscard]] virtual void* GetNativeHandle() const = 0;
	};

	class ICopyCommandList : public ICommandList
    {
    public:
        ICopyCommandList() = default;
        ~ICopyCommandList() override = default;

        ICopyCommandList(const ICopyCommandList&) = delete;
        ICopyCommandList& operator=(const ICopyCommandList&) = delete;
        ICopyCommandList(ICopyCommandList&&) = delete;
        ICopyCommandList& operator=(ICopyCommandList&&) = delete;

        /// @brief Record a buffer-to-buffer copy command
        virtual void CopyBuffer(IGraphicsBuffer* src, uint64_t srcOffset,
                                IGraphicsBuffer* dst, uint64_t dstOffset,
                                uint64_t size) = 0;

        /// @brief Record a buffer-to-texture copy command
        virtual void CopyBufferToTexture(IGraphicsBuffer* src, uint64_t srcOffset,
                                        IGraphicsTexture* dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ,
                                        uint32_t width, uint32_t height, uint32_t depth) = 0;

        /// @brief Record a texture-to-buffer copy command
        virtual void CopyTextureToBuffer(IGraphicsTexture* src, uint32_t srcX, uint32_t srcY, uint32_t srcZ,
                                        IGraphicsBuffer* dst, uint64_t dstOffset,
                                        uint32_t width, uint32_t height, uint32_t depth) = 0;

        /// @brief Record a texture-to-texture copy command
        virtual void CopyTexture(IGraphicsTexture* src, uint32_t srcX, uint32_t srcY, uint32_t srcZ,
                                 IGraphicsTexture* dst, uint32_t dstX, uint32_t dstY, uint32_t dstZ,
                                 uint32_t width, uint32_t height, uint32_t depth) = 0;

    };

    class IComputeCommandList : public ICopyCommandList
    {
    public:
        using ICopyCommandList::ICopyCommandList;

        IComputeCommandList() = default;
        ~IComputeCommandList() override = default;

        IComputeCommandList(const IComputeCommandList&) = delete;
        IComputeCommandList& operator=(const IComputeCommandList&) = delete;
        IComputeCommandList(IComputeCommandList&&) = delete;
        IComputeCommandList& operator=(IComputeCommandList&&) = delete;

        /// @brief Record a compute dispatch command
        virtual void Dispatch(uint32_t groupCountX, uint32_t groupCountY, uint32_t groupCountZ) = 0;

        /// @brief Record a compute dispatch indirect command
        virtual void DispatchIndirect(IGraphicsBuffer* indirectArgsBuffer, uint64_t offset) = 0;

        // TODO: Finish IComputeCommandList.
    };

    class IGraphicsCommandList : public IComputeCommandList
    {
    public:
        using IComputeCommandList::IComputeCommandList;

        IGraphicsCommandList() = default;
        ~IGraphicsCommandList() override = default;

        IGraphicsCommandList(const IGraphicsCommandList&) = delete;
        IGraphicsCommandList& operator=(const IGraphicsCommandList&) = delete;
        IGraphicsCommandList(IGraphicsCommandList&&) = delete;
        IGraphicsCommandList& operator=(IGraphicsCommandList&&) = delete;

        /// @brief Record a draw command
        ///
        /// @param vertexCount Number of vertices to draw
        /// @param instanceCount Number of instances to draw
        /// @param firstVertex Index of the first vertex
        /// @param firstInstance Index of the first instance
        virtual void Draw(uint32_t vertexCount, uint32_t instanceCount,
                         uint32_t firstVertex, uint32_t firstInstance) = 0;

        /// @brief Record an indexed draw command
        ///
        /// @param indexCount Number of indices to draw
        /// @param instanceCount Number of instances to draw
        /// @param firstIndex Index of the first index
        /// @param vertexOffset Value added to each index before fetching vertex data
        /// @param firstInstance Index of the first instance
        virtual void DrawIndexed(uint32_t indexCount, uint32_t instanceCount,
                                uint32_t firstIndex, int32_t vertexOffset, uint32_t firstInstance) = 0;

        /// @brief Record an indirect draw command
        /// @param indirectArgsBuffer Buffer containing draw arguments (must be created with indirect usage flag)
        /// @param offset Byte offset into the buffer where draw arguments start
        virtual void DrawIndirect(IGraphicsBuffer* indirectArgsBuffer, uint64_t offset) = 0;

        /// @brief Record an indexed indirect draw command
        ///
        /// @param indirectArgsBuffer Buffer containing draw arguments (must be created with indirect usage flag)
        /// @param offset Byte offset into the buffer where draw arguments start
        virtual void DrawIndexedIndirect(IGraphicsBuffer* indirectArgsBuffer, uint64_t offset) = 0;

        /// @brief Set vertex buffer
        /// @param slot Binding slot for the vertex buffer
        /// @param buffer Vertex buffer to bind
        /// @param offset Byte offset into the buffer
        virtual void SetVertexBuffer(uint32_t slot, IGraphicsBuffer* buffer, uint64_t offset = 0) = 0;

        /// @brief Set index buffer
        ///
        /// @param buffer Index buffer to bind
        /// @param offset Byte offset into the buffer
        virtual void SetIndexBuffer(IGraphicsBuffer* buffer, uint64_t offset = 0) = 0;

        /// @brief Set viewport
        ///
        /// @param x Top-left x coordinate of the viewport
        /// @param y Top-left y coordinate of the viewport
        /// @param width Width of the viewport
        /// @param height Height of the viewport
        /// @param minDepth Minimum depth value (0.0 to 1.0)
        /// @param maxDepth Maximum depth value (0.0 to 1.0)
        virtual void SetViewport(float x, float y, float width, float height, float minDepth, float maxDepth) = 0;

        /// @brief Set scissor rectangle
        ///
        /// @param x Top-left x coordinate of the scissor rectangle
        /// @param y Top-left y coordinate of the scissor rectangle
        /// @param width Width of the scissor rectangle
        /// @param height Height of the scissor rectangle
        virtual void SetScissor(int32_t x, int32_t y, uint32_t width, uint32_t height) = 0;

        /// @brief Begin render pass
        ///
        /// @param framebuffer Framebuffer to render to (must be compatible with the currently bound pipeline's render targets)
        virtual void BeginRenderPass(IFramebuffer* framebuffer) = 0;

        /// @brief End render pass
        virtual void EndRenderPass() = 0;

        /// @brief Bind graphics pipeline
        ///
        /// @param pipeline Pipeline to bind (must be a graphics pipeline compatible with the current render pass)
        virtual void BindPipeline(IPipeline* pipeline) = 0;
    };

	/// @brief Abstract command queue for command submission
	class ICommandQueue
	{
	public:
        ICommandQueue() = default;
		virtual ~ICommandQueue() = default;

		ICommandQueue(const ICommandQueue&) = delete;
		ICommandQueue& operator=(const ICommandQueue&) = delete;
        ICommandQueue(ICommandQueue&&) = delete;
        ICommandQueue& operator=(ICommandQueue&&) = delete;

		/// @brief Get queue type
		[[nodiscard]] virtual EQueueType GetQueueType() const = 0;

		/// @brief Submit commands for execution
		/// @param commands Opaque command buffer handle
		virtual void Submit(void* commands) = 0;

		/// @brief Wait for all commands to complete
		virtual void WaitIdle() = 0;

		/// @brief Get native handle (API-specific)
		[[nodiscard]] virtual void* GetNativeHandle() const = 0;
	};

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
		[[nodiscard]] virtual std::shared_ptr<IGraphicsTexture> CreateTexture(
			const TextureDescriptor& descriptor) = 0;

		// ========================================================================
		// Data Transfer
		// ========================================================================

		/// @brief Write data to a buffer
		/// @param buffer Target buffer
		/// @param data Source data
		/// @param size Data size in bytes
		/// @param offset Offset in buffer
		virtual void WriteBuffer(IGraphicsBuffer* buffer, const void* data,
								 uint64_t size, uint64_t offset = 0) = 0;

		/// @brief Write data to a texture
		/// @param texture Target texture
		/// @param data Source data
		/// @param width Width in pixels
		/// @param height Height in pixels
		/// @param mipLevel Target mip level
		virtual void WriteTexture(IGraphicsTexture* texture, const void* data,
								  uint32_t width, uint32_t height, uint32_t mipLevel = 0) = 0;

		// ========================================================================
		// Queue Management
		// ========================================================================

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
		virtual void* BeginFrame() = 0;

		/// @brief End frame and present
		virtual void EndFrame() = 0;

		// ========================================================================
		// Utility
		// ========================================================================

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

	// ============================================================================
	// Factory
	// ============================================================================

	/// @brief Graphics device factory
	class GraphicsDeviceFactory
	{
	public:
		/// @brief Create a graphics device
		/// @param api Graphics API to use
		/// @param windowHandle Platform window handle (e.g., SDL_Window*)
		/// @return Created device, or nullptr on failure
		[[nodiscard]] static std::unique_ptr<IGraphicsDevice> CreateDevice(
			EGraphicsAPI api, void* windowHandle);
	};

} // namespace Hush::Graphics
