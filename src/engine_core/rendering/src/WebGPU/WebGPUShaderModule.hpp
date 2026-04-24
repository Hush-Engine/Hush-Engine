/*! \file WebGPUShaderModule.hpp
	\author Alan Ramirez Herrera
	\date 2026-02-17
	\brief WebGPU implementation of IShaderModule interface.
*/
#pragma once

#include "../RHI/IShaderModule.hpp"
#include "Logger.hpp"
#include "Profiling.hpp"
#include <string>
#include <webgpu/webgpu.hpp>

namespace Hush::Graphics
{
	/// @brief WebGPU implementation of a compiled shader module.
	///
	/// Wraps a wgpu::ShaderModule that was created from WGSL source text.
	/// The module is immutable after creation and can be shared across
	/// multiple pipelines that reference the same shader stage.
	class WebGPUShaderModule final : public IShaderModule
	{
	public:
		/// @brief Construct a WebGPU shader module from a descriptor and device.
		///
		/// The descriptor's bytecode.sourceText must contain valid WGSL source.
		/// The module is created immediately during construction.
		///
		/// @param device The WebGPU device to create the module on.
		/// @param descriptor The shader module descriptor with WGSL source and metadata.
		WebGPUShaderModule(wgpu::Device device, const ShaderModuleDescriptor &descriptor)
			: m_device(device),
			  m_stage(descriptor.stage),
			  m_entryPoint(descriptor.entryPoint),
			  m_debugName(descriptor.debugName)
		{
			CreateModule(descriptor);
		}

		~WebGPUShaderModule() override
		{
			if (m_module != nullptr)
			{
				m_module.release();
				m_module = nullptr;
			}
		}

		WebGPUShaderModule(const WebGPUShaderModule &) = delete;
		WebGPUShaderModule &operator=(const WebGPUShaderModule &) = delete;

		WebGPUShaderModule(WebGPUShaderModule &&) = delete;
		WebGPUShaderModule &operator=(WebGPUShaderModule &&) = delete;

		/// @brief Get the shader stage this module was compiled for.
		[[nodiscard]]
		EShaderStage GetStage() const override
		{
			return m_stage;
		}

		/// @brief Get the entry point name.
		[[nodiscard]]
		std::string_view GetEntryPoint() const override
		{
			return m_entryPoint;
		}

		/// @brief Check if the underlying wgpu::ShaderModule was created successfully.
		[[nodiscard]]
		bool IsValid() const override
		{
			return m_module != nullptr;
		}

		/// @brief Get the native wgpu::ShaderModule handle.
		///
		/// @return Pointer to the internal wgpu::ShaderModule. Ownership is NOT
		///         transferred; this object retains ownership.
		[[nodiscard]]
		void *GetNativeHandle() const override
		{
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-const-cast)
			return static_cast<void *>(&const_cast<WebGPUShaderModule *>(this)->m_module);
		}

		/// @brief Get the underlying wgpu::ShaderModule directly.
		[[nodiscard]]
		wgpu::ShaderModule GetModule() const
		{
			return m_module;
		}

		/// @brief Get the debug name assigned at creation time.
		[[nodiscard]]
		const std::string &GetDebugName() const
		{
			return m_debugName;
		}

	private:
		/// @brief Create the wgpu::ShaderModule from the descriptor's WGSL source.
		void CreateModule(const ShaderModuleDescriptor &descriptor)
		{
			ZoneScoped;
			if (m_device == nullptr)
			{
				return;
			}

			// WebGPU requires WGSL source text
			if (!std::holds_alternative<ShaderBytecode::TextContent>(descriptor.bytecode.content))
			{
				Hush::LogFormat(Hush::ELogLevel::Error,
								"[WebGPUShaderModule] Shader bytecode does not contain WGSL source text.");
				return;
			}
			const auto &content = std::get<ShaderBytecode::TextContent>(descriptor.bytecode.content);
			const std::string &wgslSource = content.sourceText;

			wgpu::ShaderSourceWGSL wgslSourceDesc{};
			wgslSourceDesc.code = WGPUStringView{wgslSource.c_str(), wgslSource.size()};
			wgslSourceDesc.chain.sType = WGPUSType::WGPUSType_ShaderSourceWGSL;

			wgpu::ShaderModuleDescriptor moduleDesc{};
			moduleDesc.nextInChain = &wgslSourceDesc.chain;

			if (!m_debugName.empty())
			{
				moduleDesc.label = WGPUStringView{m_debugName.c_str(), m_debugName.size()};
			}

			m_module = m_device.createShaderModule(moduleDesc);
		}

		/// @brief The WebGPU device that owns this module.
		wgpu::Device m_device;

		/// @brief The compiled WebGPU shader module.
		wgpu::ShaderModule m_module = nullptr;

		/// @brief The shader stage this module was compiled for.
		EShaderStage m_stage = EShaderStage::Vertex;

		/// @brief The entry point function name within the module.
		std::string m_entryPoint;

		/// @brief Optional debug name.
		std::string m_debugName;
	};

} // namespace Hush::Graphics
