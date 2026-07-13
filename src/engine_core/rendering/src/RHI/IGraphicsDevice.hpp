/*! \file IGraphicsDevice.hpp
	\author Alan Ramirez Herrera
	\date 2026-02-17
	\brief Graphics device interface for cross-API abstraction
*/
#pragma once

#include "GraphicsTypes.hpp"
#include "IGraphicsBuffer.hpp"
#include "IGraphicsTexture.hpp"
#include "ISampler.hpp"
#include "ICommandQueue.hpp"
#include "ICommandList.hpp"
#include "IFence.hpp"
#include "IShaderModule.hpp"
#include "IPipeline.hpp"
#include "IBindGroup.hpp"
#include "PipelineDescriptor.hpp"
#include "ShaderCompiler.hpp"
#include "Logger.hpp"
#include <memory>
#include <functional>

namespace Hush::Graphics
{
	/// @brief Abstract graphics device interface
	/// Provides a unified API for creating resources and submitting commands
	class IGraphicsDevice
	{
	public:
		IGraphicsDevice() noexcept = default;

		virtual ~IGraphicsDevice() = default;

		IGraphicsDevice(const IGraphicsDevice &) = delete;
		IGraphicsDevice &operator=(const IGraphicsDevice &) = delete;
		IGraphicsDevice(IGraphicsDevice &&) = delete;
		IGraphicsDevice &operator=(IGraphicsDevice &&) = delete;

		/// @brief Get the graphics API backend
		[[nodiscard]]
		virtual EGraphicsAPI GetAPI() const = 0;

		/// @brief Get device capabilities
		[[nodiscard]]
		virtual GraphicsDeviceCapabilities GetCapabilities() const = 0;

		/// @brief Check if device is initialized
		[[nodiscard]]
		virtual bool IsInitialized() const = 0;

		/// @brief Create a buffer
		/// @param descriptor Buffer creation parameters
		/// @return Created buffer, or nullptr on failure
		[[nodiscard]]
		virtual std::unique_ptr<IGraphicsBuffer> CreateBuffer(const BufferDescriptor &descriptor) = 0;

		/// @brief Write data from the CPU into a GPU buffer.
		///
		/// This is the primary mechanism for uploading uniform data, vertex
		/// data, or any other CPU-side payload into a GPU buffer.  The buffer
		/// must have been created with appropriate usage flags (e.g.
		/// EBufferUsage::Uniform with EMemoryAccess::CPUWrite).
		///
		/// On WebGPU this maps to wgpu::Queue::writeBuffer().
		/// On Vulkan/D3D12 this maps to a staging upload or direct map+copy.
		///
		/// @param buffer  The destination GPU buffer.
		/// @param offset  Byte offset into the buffer to start writing at.
		/// @param data    Pointer to the source CPU data.
		/// @param size    Number of bytes to write.
		virtual void WriteBuffer(IGraphicsBuffer *buffer, uint64_t offset, const void *data, uint64_t size) = 0;

		/// @brief Dynamically resizes the GPU-side buffer
		/// this implies destroying the current buffer and creating a new one with
		/// an identical descriptor expanded in size
		virtual void ResizeBuffer(size_t size, IGraphicsBuffer *buffer) = 0;

		/// @brief Create a texture
		/// @param descriptor Texture creation parameters
		/// @return Created texture, or nullptr on failure
		[[nodiscard]]
		virtual std::unique_ptr<IGraphicsTexture> CreateTexture(const TextureDescriptor &descriptor) = 0;

		/// @brief Create a sampler object.
		///
		/// Samplers control how textures are sampled in shaders: filtering
		/// modes, address (wrap) modes, LOD clamping, comparison function,
		/// and anisotropy.  Samplers are immutable once created.
		///
		/// @param descriptor Sampler creation parameters.
		/// @return Created sampler, or nullptr on failure.
		[[nodiscard]]
		virtual std::unique_ptr<ISampler> CreateSampler(const SamplerDescriptor &descriptor) = 0;

		/// @brief Create a shader module from compiled bytecode or source text.
		///
		/// The descriptor should contain either binary bytecode (SPIR-V, DXIL)
		/// or source text (WGSL) depending on the backend, as produced by the
		/// Slang-based ShaderCompiler.
		///
		/// @param descriptor Shader module creation parameters (stage, entry
		///                   point, bytecode/source).
		/// @return Created shader module, or nullptr on failure.
		[[nodiscard]]
		virtual std::unique_ptr<IShaderModule> CreateShaderModule(const ShaderModuleDescriptor &descriptor) = 0;

		/// @brief Create a graphics (rasterization) pipeline.
		///
		/// The descriptor must reference valid shader modules (vertex + fragment)
		/// and at least one color target.  Shader modules must have been created
		/// by this same device.
		///
		/// @param descriptor Graphics pipeline creation parameters.
		/// @return Created pipeline, or nullptr on failure.
		[[nodiscard]]
		virtual std::unique_ptr<IGraphicsPipeline> CreateGraphicsPipeline(
			const GraphicsPipelineDescriptor &descriptor) = 0;

		/// @brief Create a compute pipeline.
		///
		/// The descriptor must reference a valid compute shader module created
		/// by this same device.
		///
		/// @param descriptor Compute pipeline creation parameters.
		/// @return Created pipeline, or nullptr on failure.
		[[nodiscard]]
		virtual std::unique_ptr<IComputePipeline> CreateComputePipeline(
			const ComputePipelineDescriptor &descriptor) = 0;

		/// @brief Create a bind group layout.
		///
		/// A bind group layout declares the expected shape (binding indices,
		/// resource types, shader stage visibility) of a set of resource
		/// bindings.  Layouts are used at pipeline creation time and when
		/// creating bind group instances.
		///
		/// @param descriptor Bind group layout creation parameters.
		/// @return Created bind group layout, or nullptr on failure.
		[[nodiscard]]
		virtual std::unique_ptr<IBindGroupLayout> CreateBindGroupLayout(
			const BindGroupLayoutDescriptor &descriptor) = 0;

		/// @brief Create a bind group (a concrete set of resource bindings).
		///
		/// A bind group is an immutable collection of buffer, texture, and
		/// sampler bindings that can be set on a command list before draw or
		/// dispatch calls.  It must conform to the layout specified in the
		/// descriptor.
		///
		/// @param descriptor Bind group creation parameters (layout + entries).
		/// @return Created bind group, or nullptr on failure.
		[[nodiscard]]
		virtual std::unique_ptr<IBindGroup> CreateBindGroup(const BindGroupDescriptor &descriptor) = 0;

		/// @brief Create a copy command list
		/// @return Created command list, or nullptr on failure
		[[nodiscard]]
		virtual std::unique_ptr<ICopyCommandList> CreateCopyCommandList() = 0;

		/// @brief Create a compute command list
		/// @return Created command list, or nullptr on failure
		[[nodiscard]]
		virtual std::unique_ptr<IComputeCommandList> CreateComputeCommandList() = 0;

		/// @brief Create a graphics command list
		/// @return Created command list, or nullptr on failure
		[[nodiscard]]
		virtual std::unique_ptr<IGraphicsCommandList> CreateGraphicsCommandList() = 0;

		/// @brief Create a timeline fence for cross-queue GPU synchronization.
		///
		/// Timeline fences hold a monotonically increasing uint64 value. Queues
		/// can signal the fence to a value after completing work, and other queues
		/// (or the CPU) can wait until the fence reaches a required value.
		///
		/// The render graph creates one fence per queue and uses the SSIS algorithm
		/// to determine the minimal set of signal/wait pairs needed to respect
		/// cross-queue dependencies.
		///
		/// @param initialValue The starting value of the fence (typically 0).
		/// @return Created fence, or nullptr on failure.
		[[nodiscard]]
		virtual std::unique_ptr<IFence> CreateFence(uint64_t initialValue = 0) = 0;

		/// @brief Get the main graphics queue
		/// @note This might return the same queue as GetComputeQueue or GetTransferQueue if the API doesn't support
		/// separate queues
		///       Which is the case for WebGPU. To check if separate queues are supported, use the capabilities struct.
		[[nodiscard]]
		virtual ICommandQueue *GetGraphicsQueue() = 0;

		/// @brief Get a compute queue.
		///
		/// On backends that don't expose multiple queues (e.g. WebGPU),
		/// this will return the same queue as GetGraphicsQueue().
		[[nodiscard]]
		virtual ICommandQueue *GetComputeQueue() = 0;

		/// @brief Get a transfer queue.
		///
		/// On backends that don't expose multiple queues (e.g. WebGPU),
		/// this will return the same queue as GetGraphicsQueue().
		[[nodiscard]]
		virtual ICommandQueue *GetTransferQueue() = 0;

		/// @brief Get a command queue by queue type.
		///
		/// Convenience method used by the render graph to map EPassType / queue
		/// index to the corresponding queue. Backends that share a single queue
		/// for multiple types (e.g. WebGPU) may return the same pointer for
		/// different queue types.
		///
		/// @param type The queue type to retrieve.
		/// @return Pointer to the requested queue, never nullptr (falls back to
		///         the graphics queue if the requested type is not available).
		[[nodiscard]]
		virtual ICommandQueue *GetQueueForType(EQueueType type)
		{
			switch (type)
			{
			case EQueueType::Compute:
				return GetComputeQueue();
			case EQueueType::Transfer:
				return GetTransferQueue();
			case EQueueType::Graphics:
			default:
				return GetGraphicsQueue();
			}
		}

		/// @brief Map a render-graph pass type to the physical queue index used
		///        by this device.
		///
		/// The render graph assigns each pass a queue index that drives
		/// dependency detection, SSIS culling, fence allocation, and execution
		/// plan generation.  On multi-queue backends (D3D12, Vulkan) the
		/// default mapping is a 1:1 correspondence with EPassType ordinal
		/// values (Graphics=0, Compute=1, Transfer=2).
		///
		/// Single-queue backends (WebGPU) should override this to collapse
		/// all pass types onto queue index 0, which eliminates unnecessary
		/// cross-queue synchronisation, fence pairs, and execution plans.
		///
		/// @param passType The logical pass type declared by the user.
		/// @return The physical queue index to use in the render graph.
		[[nodiscard]]
		virtual uint32_t MapPassTypeToQueueIndex(EQueueType passType) const
		{
			return static_cast<uint32_t>(passType);
		}

		/// @brief Get the bitmask of EResourceState values that the given queue
		///        type can transition resources to.
		///
		/// This is used by the render graph's transition rerouting logic to
		/// determine the "most competent queue" — the queue capable of performing
		/// a required state transition. When a receiving queue cannot perform a
		/// transition (e.g. compute queue cannot transition to PixelShaderAccess),
		/// the transition is rerouted to the most competent queue (usually graphics).
		///
		/// @param type The queue type to query.
		/// @return Bitmask of supported EResourceState values for transitions.
		[[nodiscard]]
		virtual uint32_t GetQueueSupportedStates(EQueueType type) const
		{
			auto caps = GetCapabilities();
			switch (type)
			{
			case EQueueType::Compute:
				return caps.computeQueueSupportedStates;
			case EQueueType::Transfer:
				return caps.transferQueueSupportedStates;
			case EQueueType::Graphics:
			default:
				return caps.graphicsQueueSupportedStates;
			}
		}

		/// @brief Begin a new frame
		/// @return Swapchain texture handle, or nullptr on failure
		virtual void BeginFrame() = 0;

		/// @brief End frame and present
		virtual void EndFrame() = 0;

		/// @brief Get the current frame's swapchain texture
		[[nodiscard]]
		virtual IGraphicsTexture *GetCurrentFrameTexture() const = 0;

		/// @brief Resize the swapchain
		/// @param width New width
		/// @param height New height
		virtual void Resize(uint32_t width, uint32_t height) = 0;

		/// @brief Add a function to the deletion queue for deferred cleanup
		/// @param deleteFunc Deletion function
		virtual void AddToDeletionQueue(std::function<void()> &&deleteFunc) = 0;

		/// @brief Flush deletion queue
		virtual void FlushDeletionQueue() = 0;

		/// @brief Get native device handle (API-specific)
		[[nodiscard]]
		virtual void *GetNativeHandle() const = 0;

		[[nodiscard]]
		virtual ETextureFormat GetPreferredSwapchainFormat() const = 0;
	};

} // namespace Hush::Graphics
