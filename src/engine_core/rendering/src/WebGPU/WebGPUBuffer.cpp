/*! \file WebGPUBuffer.cpp
	\author Alan Ramirez Herrera
	\date 2026-02-17
	\brief WebGPU buffer implementation
*/
#include "WebGPUBuffer.hpp"
#include "RHI/GraphicsTypes.hpp"
#include "WebGPU/WebGPUGraphicsDevice.hpp"
#include "Assertions.hpp"
#include "Logger.hpp"
#ifndef HUSH_PLATFORM_EMSCRIPTEN
#include "Profiling.hpp"
#include <webgpu.h>
#endif
#include <webgpu/webgpu.hpp>
#include <functional>
#include <thread>

namespace Hush::Graphics
{

	WebGPUBuffer::WebGPUBuffer(wgpu::Buffer buffer, wgpu::Instance instance, const BufferDescriptor &desc)
		: m_buffer(buffer),
		  m_instance(instance),
		  m_descriptor(desc)
	{
	}

	WebGPUBuffer::~WebGPUBuffer()
	{
		if (m_mappedData != nullptr)
		{
			Unmap();
		}
		m_buffer.destroy();
	}

	void WebGPUBuffer::Destroy()
	{
		if (m_mappedData != nullptr)
		{
			Unmap();
		}
		m_buffer.destroy();
	}

	void *WebGPUBuffer::Map(Graphics::IGraphicsDevice *device)
	{
#ifndef HUSH_PLATFORM_EMSCRIPTEN
		ZoneScoped;
#endif
		(void)device;
		if (m_mappedData != nullptr)
		{
			return m_mappedData;
		}

		bool success = false;
		wgpu::BufferMapCallbackInfo callbackInfo{};
		callbackInfo.setDefault();

		callbackInfo.userdata1 = &success;
		callbackInfo.callback = [](WGPUMapAsyncStatus status, WGPUStringView message,
								   [[maybe_unused]] WGPU_NULLABLE void *userdata1,
								   [[maybe_unused]]
								   WGPU_NULLABLE void *userdata2) {
			bool *successPtr = static_cast<bool *>(userdata1);
			*successPtr = (status == WGPUMapAsyncStatus::WGPUMapAsyncStatus_Success);
			if (status == WGPUMapAsyncStatus::WGPUMapAsyncStatus_Success)
			{
				// Mapping succeeded, you can now access the buffer data
				LogTrace("WebGPU buffer mapping succeeded");
			}
			else
			{
				Hush::LogFormat(ELogLevel::Error, "WebGPU buffer mapping failed: {:.{}}", message.data,
								static_cast<int>(message.length));
			}
		};

		wgpu::MapMode mapMode = wgpu::MapMode::None;

		if (HasFlag(m_descriptor.memoryAccess, EMemoryAccess::CPURead))
		{
			mapMode =
				static_cast<wgpu::MapMode>(static_cast<uint32_t>(mapMode) | static_cast<uint32_t>(wgpu::MapMode::Read));
		}
		if (HasFlag(m_descriptor.memoryAccess, EMemoryAccess::CPUWrite))
		{
			mapMode = static_cast<wgpu::MapMode>(static_cast<uint32_t>(mapMode) |
												 static_cast<uint32_t>(wgpu::MapMode::Write));
		}

// In WGPU-native, mapAsync is not async, so we don't need to wait.
// However, in the web, mapAsync is truly async, so we need to wait for the callback to be invoked before we can access
// the mapped data.
#if !defined(WEBGPU_BACKEND_WGPU) && !defined(WEBGPU_BACKEND_EMDAWNWEBGPU)
		auto mapBufferFuture = m_buffer.mapAsync(mapMode, 0, m_descriptor.size, callbackInfo);

		WGPUFutureWaitInfo futureInfo = {};
		futureInfo.future = mapBufferFuture;
		uint64_t timeoutNS = 200 * 1000; // 200 ms
		WGPUWaitStatus status = wgpuInstanceWaitAny(m_instance, 1, &futureInfo, timeoutNS);
		if (status != WGPUWaitStatus::WGPUWaitStatus_Success)
		{
			Hush::LogFormat(ELogLevel::Error, "WebGPU buffer mapping wait failed with status: %d",
							static_cast<int>(status));
			return nullptr;
		}

#elif defined(WEBGPU_BACKEND_WGPU)
		m_buffer.mapAsync(mapMode, 0, m_descriptor.size, callbackInfo);
		// On WGPU-native, we need to poll events to ensure the mapAsync callback is processed.
		// Note: wgpuInstanceWaitAny is unimplemented in this prebuilt wgpu-native build (panics
		// with "not implemented" at src\unimplemented.rs), so a future-based wait is not usable.
		[[maybe_unused]]
		auto *webGpuGraphicsDevice = dynamic_cast<WebGPUGraphicsDevice *>(device);
		webGpuGraphicsDevice->PollEvents();
#elif defined(WEBGPU_BACKEND_EMDAWNWEBGPU)
		m_buffer.mapAsync(mapMode, 0, m_descriptor.size, wgpu::CallbackMode::AllowProcessEvents,
						  [&success](wgpu::MapAsyncStatus status, WGPUStringView message) {
							  success = (status == wgpu::MapAsyncStatus::Success);
							  if (!success)
							  {
								  Hush::LogFormat(ELogLevel::Error, "WebGPU buffer mapping failed: {:.{}}",
												  message.data, static_cast<int>(message.length));
							  }
						  });
		while (!success)
		{
			emscripten_sleep(5);		// Sleep for 5 ms before checking again
			m_instance.processEvents(); // Process any pending WebGPU events, including the mapAsync callback
		}
#endif

		HUSH_ASSERT(success, "WebGPU buffer mapping failed: callback was not invoked with success status.");

		if (!success)
		{
			Hush::LogError("WebGPU buffer mapping failed");
			return nullptr;
		}

		m_mappedData = m_buffer.getMappedRange(0, m_descriptor.size);

		LogFormat(ELogLevel::Trace, "WebGPU buffer mapped size={} ptr={} on thread {}", m_descriptor.size,
				  static_cast<void *>(m_mappedData), std::hash<std::thread::id>{}(std::this_thread::get_id()));

		return m_mappedData;
	}

	void WebGPUBuffer::Unmap()
	{
		if (m_mappedData == nullptr)
		{
			return;
		}

		LogFormat(ELogLevel::Trace, "WebGPU buffer unmapping size={} ptr={} on thread {}", m_descriptor.size,
				  static_cast<void *>(m_mappedData), std::hash<std::thread::id>{}(std::this_thread::get_id()));

		m_buffer.unmap();
		m_mappedData = nullptr;
	}

	void *WebGPUBuffer::GetNativeHandle() const
	{
		return static_cast<void *>(static_cast<WGPUBuffer>(m_buffer));
	}

} // namespace Hush::Graphics
