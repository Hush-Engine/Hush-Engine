/*! \file WebGPURenderer.cpp
	\author Alan Ramirez Herrera
	\date 2025-11-16
	\brief WebGPU implementation for rendering
*/
#include "WebGPURenderer.hpp"

#include "Assertions.hpp"
#include <typeutils/TypeUtils.hpp>
#include <webgpu.h>
#include <webgpu/webgpu.hpp>
#include "Shared/GPUMeshBuffers.hpp"
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>
#include <Shared/ShaderMaterial.hpp>
#include "Shared/DirectionalLight.hpp"
#include <glm/gtx/string_cast.hpp>
#include "Renderer.hpp"
#include "Shared/GpuAllocatedImage.hpp"
#include "Shared/MaterialOptions.hpp"
#include "Shared/Types/Color.hpp"
#include <cstdint>
#include <magic_enum/magic_enum.hpp>
#include "../../core/src/Components/WorldTransform.hpp"
#include "../../core/src/Scene.hpp"

Hush::WebGPURenderer::WebGPURenderer(void *windowContext, ERenderingBackend type)
	: IRenderer(windowContext, type)
{
	LogTrace("Initializing WebGPU");

#if HUSH_PLATFORM_EMSCRIPTEN
	m_instance = wgpu::Instance(wgpuCreateInstance(nullptr));
#else
	wgpu::InstanceDescriptor instanceDescriptor = {};
	instanceDescriptor.nextInChain = nullptr;
	m_instance = wgpu::Instance(wgpuCreateInstance(&instanceDescriptor));
#endif

	HUSH_ASSERT(m_instance, "Failed to create WebGPU instance");

	wgpu::RequestAdapterOptions adapterOptions = {};
	adapterOptions.backendType = wgpu::BackendType::D3D12;
	adapterOptions.powerPreference = wgpu::PowerPreference::HighPerformance;
	adapterOptions.nextInChain = nullptr;
	adapterOptions.featureLevel = wgpu::FeatureLevel::Core;

	m_adapter = m_instance->requestAdapter(adapterOptions);

	HUSH_ASSERT(m_adapter, "Failed to create WebGPU adapter");
}
