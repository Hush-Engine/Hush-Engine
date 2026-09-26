/*! \file WebGPUCommandQueue.cpp
	\author Alan Ramirez Herrera
	\date 2026-02-17
	\brief Single-queue ordering with genuine GPU completion notifications.
*/
#include "WebGPUCommandQueue.hpp"
#include "WebGPUCommandList.hpp"
#include "WebGPUFence.hpp"
#include "Profiling.hpp" // IWYU pragma: keep
#include <stdexcept>

namespace Hush::Graphics
{
	WebGPUCommandQueue::WebGPUCommandQueue(wgpu::Device device, wgpu::Instance instance, wgpu::Queue queue,
										   EQueueType type)
		: m_device(device),
		  m_instance(instance),
		  m_queue(queue),
		  m_queueType(type)
	{
	}

	void WebGPUCommandQueue::Submit(std::span<ICommandList *> commandLists)
	{
		SubmitCommands(commandLists);
	}

	void WebGPUCommandQueue::SubmitCommands(std::span<ICommandList *const> commandLists)
	{
		ZoneScoped;
		if (commandLists.empty())
		{
			return;
		}
		std::vector<wgpu::CommandBuffer> buffers;
		buffers.reserve(commandLists.size());
		for (auto *cmd : commandLists)
		{
			if (auto *copy = dynamic_cast<WebGPUCopyCommandList *>(cmd))
			{
				buffers.push_back(copy->GetCommandBuffer());
			}
			else if (auto *compute = dynamic_cast<WebGPUComputeCommandList *>(cmd))
			{
				buffers.push_back(compute->GetCommandBuffer());
			}
			else if (auto *graphics = dynamic_cast<WebGPUGraphicsCommandList *>(cmd))
			{
				buffers.push_back(graphics->GetCommandBuffer());
			}
			else
			{
				throw std::invalid_argument("Unsupported WebGPU command list");
			}
		}
		m_queue.submit(buffers.size(), buffers.data());
	}

	void WebGPUCommandQueue::SubmitBatched(const SubmitInfo &info)
	{
		ZoneScoped;
		for (const auto &wait : info.waitFences)
		{
			Wait(wait.fence, wait.value);
		}
		for (const auto &signal : info.signalFences)
		{
			const auto *fence = dynamic_cast<WebGPUFence *>(signal.fence);
			if (fence == nullptr || !fence->BelongsTo(m_queue) || signal.value <= fence->GetPendingValue())
			{
				throw std::invalid_argument("Invalid WebGPU timeline signal");
			}
		}
		SubmitCommands(info.commandLists);
		for (const auto &signal : info.signalFences)
		{
			Signal(signal.fence, signal.value);
		}
	}

	void WebGPUCommandQueue::Signal(IFence *fence, uint64_t value)
	{
		auto *webgpu = dynamic_cast<WebGPUFence *>(fence);
		if (webgpu == nullptr || !webgpu->BelongsTo(m_queue))
		{
			throw std::invalid_argument("WebGPU cannot signal a foreign fence");
		}
		webgpu->SignalGPU(value);
	}

	void WebGPUCommandQueue::Wait(IFence *fence, uint64_t value)
	{
		if (fence != nullptr && fence->GetCompletedValue() >= value)
		{
			return;
		}
		const auto *webgpu = dynamic_cast<WebGPUFence *>(fence);
		if (webgpu == nullptr || !webgpu->BelongsTo(m_queue) || value > webgpu->GetPendingValue())
		{
			throw std::invalid_argument("WebGPU cannot wait for foreign or not-yet-submitted work");
		}
		// This exact point was already submitted on this queue: FIFO supplies the
		// GPU dependency. Never turn a queue dependency into a per-frame CPU wait.
	}

	void WebGPUCommandQueue::WaitIdle()
	{
		WebGPUFence completion(m_device, m_instance, m_queue);
		completion.SignalGPU(1);
		if (!completion.WaitCPU(1))
		{
			throw std::runtime_error("WebGPU queue completion failed");
		}
	}

	void *WebGPUCommandQueue::GetNativeHandle() const
	{
		return static_cast<void *>(static_cast<WGPUQueue>(m_queue));
	}

	wgpu::CommandEncoder WebGPUCommandQueue::CreateEncoder(const char * /*label*/)
	{
		return {};
	}
} // namespace Hush::Graphics
