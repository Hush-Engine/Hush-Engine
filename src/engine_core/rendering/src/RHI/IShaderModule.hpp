/*! \file IShaderModule.hpp
	\author Alan Ramirez Herrera
	\date 2025-02-17
	\brief Abstract interface for a compiled shader module (one stage)

	An IShaderModule represents a single compiled shader stage (vertex,
	fragment, compute, etc.) that can be used when constructing a pipeline.

	The shader module is created by the graphics device from compiled shader
	bytecode or source text (depending on the backend). The Slang-based
	ShaderCompiler produces the appropriate intermediate representation for
	each backend:
	  - WGSL source string  for WebGPU
	  - SPIR-V bytecode     for Vulkan
	  - DXIL bytecode       for D3D12

	Shader modules are immutable after creation and can be shared across
	multiple pipelines that use the same stage.
*/
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace Hush::Graphics
{
	/// @brief Shader stage type
	enum class EShaderStage : uint32_t
	{
		Vertex = 0,
		Fragment,
		Compute,
		// Future: Geometry, TessControl, TessEval, Mesh, Task, RayGen, etc.
		Count
	};

	/// @brief Holds compiled shader data in a backend-appropriate format.
	///
	/// For SPIR-V / DXIL backends this contains binary bytecode.
	/// For WebGPU this contains WGSL source text.
	struct ShaderBytecode
	{
		/// @brief Binary data (SPIR-V, DXIL, etc.)
		std::vector<uint8_t> data;

		/// @brief Source text (WGSL, HLSL source, etc.)
		/// When non-empty, `data` may be empty and vice-versa depending on the
		/// target backend.
		std::string sourceText;

		/// @brief Returns true if this bytecode container has usable content.
		[[nodiscard]]
		bool IsValid() const
		{
			return !data.empty() || !sourceText.empty();
		}
	};

	/// @brief Descriptor for creating a shader module
	struct ShaderModuleDescriptor
	{
		/// @brief The shader stage this module represents
		EShaderStage stage = EShaderStage::Vertex;

		/// @brief Entry point function name within the shader
		/// Defaults to "main" for GLSL/SPIR-V convention.
		/// Slang shaders may use names like "vertexMain", "fragmentMain", etc.
		std::string entryPoint = "main";

		/// @brief Compiled shader bytecode or source
		ShaderBytecode bytecode;

		/// @brief Optional debug name
		std::string debugName;
	};

	/// @brief Abstract interface for a compiled shader module.
	///
	/// Represents a single shader stage that has been compiled and is ready
	/// to be used in pipeline creation. The underlying representation is
	/// backend-specific (e.g. wgpu::ShaderModule for WebGPU, VkShaderModule
	/// for Vulkan).
	class IShaderModule
	{
	public:
		IShaderModule() = default;
		virtual ~IShaderModule() = default;

		IShaderModule(const IShaderModule &) = delete;
		IShaderModule &operator=(const IShaderModule &) = delete;
		IShaderModule(IShaderModule &&) = delete;
		IShaderModule &operator=(IShaderModule &&) = delete;

		/// @brief Get the shader stage this module was compiled for
		[[nodiscard]]
		virtual EShaderStage GetStage() const = 0;

		/// @brief Get the entry point name
		[[nodiscard]]
		virtual std::string_view GetEntryPoint() const = 0;

		/// @brief Check if the module is valid and usable
		[[nodiscard]]
		virtual bool IsValid() const = 0;

		/// @brief Get the native API handle
		/// For WebGPU: wgpu::ShaderModule*
		/// For Vulkan: VkShaderModule*
		[[nodiscard]]
		virtual void *GetNativeHandle() const = 0;
	};

} // namespace Hush::Graphics
