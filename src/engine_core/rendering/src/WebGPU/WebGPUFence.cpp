/*! \file WebGPUFence.cpp
	\author Alan Ramirez Herrera
	\date 2026-02-17
	\brief GPU-backed timeline completion implementation, extracted from WebGPUFence.hpp.
*/
#include "WebGPUFence.hpp"
#include <chrono>
#include <stdexcept>
#include <thread>
#if HUSH_PLATFORM_EMSCRIPTEN
#include <emscripten.h>
#endif

using namespace Hush::Graphics;

WebGPUFence::WebGPUFence(wgpu::Device device, wgpu::Instance instance, wgpu::Queue queue, uint64_t initialValue)
	: m_device(device),
	  m_instance(instance),
	  m_queue(queue),
	  m_state(std::make_shared<CompletionState>())
{
	m_state->completed.store(initialValue);
	m_state->pending.store(initialValue);
}

uint64_t WebGPUFence::GetCompletedValue() const
{
	return m_state->completed.load(std::memory_order_acquire);
}
uint64_t WebGPUFence::GetPendingValue() const
{
	return m_state->pending.load(std::memory_order_acquire);
}

void WebGPUFence::Advance(std::atomic<uint64_t> &counter, uint64_t value)
{
	auto current = counter.load(std::memory_order_relaxed);
	while (current < value &&
		   !counter.compare_exchange_weak(current, value, std::memory_order_release, std::memory_order_relaxed))
	{
	}
}

void WebGPUFence::OnWorkDone(WGPUQueueWorkDoneStatus status, void *userdata1, void * /*userdata2*/)
{
	const std::unique_ptr<CallbackData> callback(static_cast<CallbackData *>(userdata1));
	if (status == WGPUQueueWorkDoneStatus_Success)
	{
		Advance(callback->state->completed, callback->value);
	}
	else
	{
		callback->state->failed.store(true, std::memory_order_release);
	}
}

void WebGPUFence::SignalGPU(uint64_t value)
{
	if (value <= GetPendingValue() || m_state->failed.load(std::memory_order_acquire))
	{
		throw std::invalid_argument("WebGPU fence signals must increase on a healthy timeline");
	}
	auto callback = std::make_unique<CallbackData>(CallbackData{.state = m_state, .value = value});
	WGPUQueueWorkDoneCallbackInfo info{};
	info.mode = WGPUCallbackMode_AllowProcessEvents;
	info.callback = OnWorkDone;
	info.userdata1 = callback.release(); // The API invokes exactly one success/error/cancellation callback.
	m_state->pending.store(value, std::memory_order_release);
	wgpuQueueOnSubmittedWorkDone(m_queue, info);
}

void WebGPUFence::SignalCPU(uint64_t value)
{
	if (GetPendingValue() > GetCompletedValue() || value <= GetPendingValue())
	{
		throw std::invalid_argument("CPU signals cannot bypass pending GPU work or reuse timeline values");
	}
	m_state->pending.store(value, std::memory_order_release);
	Advance(m_state->completed, value);
}

bool WebGPUFence::WaitCPU(uint64_t value, uint64_t timeoutNs)
{
	const auto start = std::chrono::steady_clock::now();
	while (GetCompletedValue() < value)
	{
		if (m_state->failed.load(std::memory_order_acquire))
		{
			return false;
		}
		const auto elapsed =
			std::chrono::duration_cast<std::chrono::nanoseconds>(std::chrono::steady_clock::now() - start);
		if (timeoutNs != std::numeric_limits<uint64_t>::max() && static_cast<uint64_t>(elapsed.count()) >= timeoutNs)
		{
			return false;
		}
#ifdef WEBGPU_BACKEND_WGPU
		m_device.poll(0u, nullptr);
#else
		m_instance.processEvents();
#endif
		if (GetCompletedValue() >= value)
		{
			return true;
		}
#if HUSH_PLATFORM_EMSCRIPTEN
		emscripten_sleep(1); // Exceptional CPU waits yield to the browser; ordinary frames never use this path.
#else
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
#endif
	}
	return true;
}
