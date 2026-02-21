/*! \file WebGPUCommandQueue.cpp
	\author Alan Ramirez Herrera
	\date 2025-01-17
	\brief WebGPU command queue implementation
*/
#include "WebGPUCommandQueue.hpp"
#include "WebGPUCommandList.hpp"
#include "WebGPUFence.hpp"
#include "Logger.hpp"

namespace Hush::Graphics
{

	WebGPUCommandQueue::WebGPUCommandQueue(wgpu::Queue queue, EQueueType type)
		: m_queue(queue),
		  m_queueType(type)
	{
	}

	void WebGPUCommandQueue::Submit(std::span<ICommandList *> commandLists)
	{
		if (commandLists.empty())
		{
			return;
		}

		std::vector<wgpu::CommandBuffer> wgpuCommandBuffers;
		wgpuCommandBuffers.reserve(commandLists.size());

		for (ICommandList *cmdList : commandLists)
		{
			if (auto *webgpuCopyCmdList = dynamic_cast<WebGPUCopyCommandList *>(cmdList); webgpuCopyCmdList != nullptr)
			{
				wgpuCommandBuffers.push_back(webgpuCopyCmdList->GetCommandBuffer());
			}
			else if (auto *webgpuComputeCmdList = dynamic_cast<WebGPUComputeCommandList *>(cmdList);
					 webgpuComputeCmdList != nullptr)
			{
				wgpuCommandBuffers.push_back(webgpuComputeCmdList->GetCommandBuffer());
			}
			else if (auto *webgpuGfxCmdList = dynamic_cast<WebGPUGraphicsCommandList *>(cmdList);
					 webgpuGfxCmdList != nullptr)
			{
				wgpuCommandBuffers.push_back(webgpuGfxCmdList->GetCommandBuffer());
			}
			else
			{
				Hush::LogError("Unsupported command list type submitted to WebGPUCommandQueue");
			}
		}

		m_queue.submit(wgpuCommandBuffers.size(), wgpuCommandBuffers.data());
	}

	void WebGPUCommandQueue::SubmitBatched(const SubmitInfo &submitInfo)
	{
		// WebGPU has a single queue, so cross-queue GPU waits don't apply.
		// We honour the contract by performing CPU-side waits on the emulated
		// fence values so that the render graph executor's ordering invariants
		// are respected even though the GPU work is already serialized.
		for (const auto &wait : submitInfo.waitFences)
		{
			if (wait.fence != nullptr)
			{
				auto *webgpuFence = dynamic_cast<WebGPUFence *>(wait.fence);
				// CPU-side spin/wait — in practice the value should already be
				// reached because WebGPU serializes everything on one queue.
				if (webgpuFence != nullptr)
				{
					webgpuFence->WaitCPU(wait.value);
				}
			}
		}

		if (!submitInfo.commandLists.empty())
		{
			// Submit expects a span of ICommandList*; the vector is contiguous.
			Submit(std::span<ICommandList *>(const_cast<ICommandList **>(submitInfo.commandLists.data()),
											 submitInfo.commandLists.size()));
		}

		for (const auto &signal : submitInfo.signalFences)
		{
			if (signal.fence != nullptr)
			{
				auto *webgpuFence = dynamic_cast<WebGPUFence *>(signal.fence);
				if (webgpuFence != nullptr)
				{
					webgpuFence->SignalCPU(signal.value);
				}
			}
		}
	}

	void WebGPUCommandQueue::Signal(IFence *fence, uint64_t value)
	{
		if (fence == nullptr)
		{
			return;
		}

		// CPU-side emulation: immediately mark the fence as signaled.
		// Because WebGPU serializes all work on a single queue the signal is
		// logically "after" all previously submitted work.
		auto *webgpuFence = dynamic_cast<WebGPUFence *>(fence);
		if (webgpuFence != nullptr)
		{
			webgpuFence->SignalCPU(value);
		}
	}

	void WebGPUCommandQueue::Wait(IFence *fence, uint64_t value)
	{
		if (fence == nullptr)
		{
			return;
		}

		// CPU-side emulation: block until the emulated fence reaches the value.
		// In practice the value should already be reached since WebGPU has only
		// one queue, but we honour the contract for correctness.
		auto *webgpuFence = dynamic_cast<WebGPUFence *>(fence);
		if (webgpuFence != nullptr)
		{
			webgpuFence->WaitCPU(value);
		}
	}

	void WebGPUCommandQueue::WaitIdle()
	{
		// WebGPU doesn't have direct waitIdle
		// Could implement with fences/callbacks if needed
	}

	void *WebGPUCommandQueue::GetNativeHandle() const
	{
		return static_cast<void *>(static_cast<WGPUQueue>(m_queue));
	}

	wgpu::CommandEncoder WebGPUCommandQueue::CreateEncoder(const char * /*label*/)
	{
		// Note: This helper method is not used in the current implementation
		// Command encoders should be created directly from the device
		return {};
	}

} // namespace Hush::Graphics
