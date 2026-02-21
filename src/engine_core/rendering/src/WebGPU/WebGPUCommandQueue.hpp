/*! \file WebGPUCommandQueue.hpp
	\author Alan Ramirez Herrera
	\date 2025-01-17
	\brief WebGPU command queue implementation
*/
#pragma once

#include "../RHI/ICommandQueue.hpp"
#include <webgpu/webgpu.hpp>

namespace Hush::Graphics
{
	/// @brief WebGPU command queue implementation
	class WebGPUCommandQueue : public ICommandQueue
	{
	public:
		WebGPUCommandQueue(wgpu::Queue queue, EQueueType type);
		~WebGPUCommandQueue() override = default;

		WebGPUCommandQueue(const WebGPUCommandQueue &) = delete;
		WebGPUCommandQueue(WebGPUCommandQueue &&) = delete;
		WebGPUCommandQueue &operator=(const WebGPUCommandQueue &) = delete;
		WebGPUCommandQueue &operator=(WebGPUCommandQueue &&) = delete;

		[[nodiscard]]
		EQueueType GetQueueType() const override
		{
			return m_queueType;
		}

		void Submit(std::span<ICommandList *> commandLists) override;

		void SubmitBatched(const SubmitInfo &submitInfo) override;

		void Signal(IFence *fence, uint64_t value) override;

		void Wait(IFence *fence, uint64_t value) override;

		void WaitIdle() override;

		[[nodiscard]]
		void *GetNativeHandle() const override;

		[[nodiscard]]
		wgpu::Queue GetQueue() const
		{
			return m_queue;
		}

		wgpu::CommandEncoder CreateEncoder(const char *label = nullptr);

	private:
		wgpu::Queue m_queue;
		EQueueType m_queueType;
	};

} // namespace Hush::Graphics
