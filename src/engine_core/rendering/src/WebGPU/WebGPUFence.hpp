/*! \file WebGPUFence.hpp
	\author Alan Ramirez Herrera
	\date 2026-02-17
	\brief Timeline completion backed by WebGPU submitted-work callbacks.
*/
#pragma once

#include "../RHI/IFence.hpp"
#include <webgpu/webgpu.hpp>
#include <atomic>
#include <memory>

namespace Hush::Graphics
{
	/// Queue submission is NOT completion. Callback storage owns its state even
	/// if the fence wrapper is destroyed. Device/instance must outlive CPU waits.
	class WebGPUFence : public IFence
	{
	public:
		WebGPUFence(wgpu::Device device, wgpu::Instance instance, wgpu::Queue queue, uint64_t initialValue = 0);
		~WebGPUFence() override = default;
		WebGPUFence(const WebGPUFence &) = delete;
		WebGPUFence &operator=(const WebGPUFence &) = delete;
		WebGPUFence(WebGPUFence &&) = delete;
		WebGPUFence &operator=(WebGPUFence &&) = delete;

		[[nodiscard]]
		uint64_t GetCompletedValue() const override;
		[[nodiscard]]
		uint64_t GetPendingValue() const override;
		bool WaitCPU(uint64_t value, uint64_t timeoutNs = std::numeric_limits<uint64_t>::max()) override;
		/// CPU-controlled timelines only; cannot bypass outstanding GPU work.
		void SignalCPU(uint64_t value) override;
		void SignalGPU(uint64_t value);
		[[nodiscard]]
		bool BelongsTo(wgpu::Queue queue) const
		{
			return m_queue == queue;
		}
		[[nodiscard]]
		void *GetNativeHandle() const override
		{
			return nullptr;
		}

	private:
		struct CompletionState
		{
			std::atomic<uint64_t> completed{0};
			std::atomic<uint64_t> pending{0};
			std::atomic<bool> failed{false};
		};
		struct CallbackData
		{
			std::shared_ptr<CompletionState> state;
			uint64_t value;
		};
		static void Advance(std::atomic<uint64_t> &counter, uint64_t value);
		static void OnWorkDone(WGPUQueueWorkDoneStatus status, void *userdata1, void *userdata2);

		wgpu::Device m_device;
		wgpu::Instance m_instance;
		wgpu::Queue m_queue;
		std::shared_ptr<CompletionState> m_state;
	};
} // namespace Hush::Graphics
