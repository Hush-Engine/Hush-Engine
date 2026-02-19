/*! \file WebGPUCommandQueue.cpp
	\author Alan Ramirez Herrera
	\date 2025-01-17
	\brief WebGPU command queue implementation
*/
#include "WebGPUCommandQueue.hpp"
#include "WebGPUCommandList.hpp"
#include "Logger.hpp"

namespace Hush::Graphics
{

WebGPUCommandQueue::WebGPUCommandQueue(wgpu::Queue queue, EQueueType type)
	: m_queue(queue)
	, m_queueType(type)
{
}

void WebGPUCommandQueue::Submit(std::span<ICommandList*> commandLists)
{
	if (commandLists.empty())
	{
		return;
	}

	std::vector<wgpu::CommandBuffer> wgpuCommandBuffers;
    wgpuCommandBuffers.reserve(commandLists.size());

    for (ICommandList* cmdList : commandLists)
    {
        if (auto* webgpuCopyCmdList = dynamic_cast<WebGPUCopyCommandList*>(cmdList); webgpuCopyCmdList != nullptr)
        {
            wgpuCommandBuffers.push_back(webgpuCopyCmdList->GetCommandBuffer());
        }
        else if (auto* webgpuComputeCmdList = dynamic_cast<WebGPUComputeCommandList*>(cmdList); webgpuComputeCmdList != nullptr)
        {
            wgpuCommandBuffers.push_back(webgpuComputeCmdList->GetCommandBuffer());
        }
        else if (auto* webgpuGfxCmdList = dynamic_cast<WebGPUGraphicsCommandList*>(cmdList); webgpuGfxCmdList != nullptr)
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

void WebGPUCommandQueue::WaitIdle()
{
	// WebGPU doesn't have direct waitIdle
	// Could implement with fences/callbacks if needed
}

void* WebGPUCommandQueue::GetNativeHandle() const
{
	return static_cast<void*>(static_cast<WGPUQueue>(m_queue));
}

wgpu::CommandEncoder WebGPUCommandQueue::CreateEncoder(const char* /*label*/)
{
	// Note: This helper method is not used in the current implementation
	// Command encoders should be created directly from the device
	return {};
}

} // namespace Hush::Graphics
