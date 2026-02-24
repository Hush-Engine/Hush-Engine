/*! \file IShaderModule.hpp
	\author Alan Ramirez Herrera
	\date 2026-02-17
	\brief Abstract interface for a compiled shader module (one stage)
*/
#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <variant>

namespace Hush::Graphics
{
	/// @brief Shader stage type
	enum class EShaderStage : uint32_t
	{
		Vertex = 0,
		Fragment,
		Compute,
		// Future: Geometry, RayTracing?
		Count
	};

	/// @brief Holds compiled shader data in a backend-appropriate format.
	///
	/// For SPIR-V / DXIL backends this contains binary bytecode.
	/// For WebGPU this contains WGSL source text.
	struct ShaderBytecode
	{
		/// Text representation of the shader content (e.g. WGSL source for WebGPU)
		struct TextContent
		{
			std::string sourceText;
		};

		/// Binary representation of the shader content (e.g. SPIR-V or DXIL bytecode for Vulkan/D3D12)
		struct BinaryContent
		{
			std::vector<uint8_t> data;
		};

		/// @brief The shader content, which can be either binary or text depending on the backend.
		std::variant<BinaryContent, TextContent, std::monostate> content;

		/// @brief Returns true if this bytecode container has usable content.
		[[nodiscard]]
		bool IsValid() const
		{
			return std::visit(
				[](const auto &value) {
					using T = std::decay_t<decltype(value)>;
					if constexpr (std::is_same_v<T, BinaryContent>)
					{
						return !value.data.empty();
					}
					else if constexpr (std::is_same_v<T, TextContent>)
					{
						return !value.sourceText.empty();
					}
					return false;
				},
				content);
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
