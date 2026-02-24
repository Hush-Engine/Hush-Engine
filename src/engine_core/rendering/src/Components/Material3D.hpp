#pragma once
#include "Shared/MaterialOptions.hpp"
#include "Shared/MaterialPass.hpp"
#include "RHI/GraphicsResources.hpp"
#include "RHI/ICommandList.hpp"
#include "RHI/MaterialInstance.hpp"
#include <cstring>
#include <string_view>
#include <string>
#include <unordered_map>
#include <vector>

namespace Hush::Graphics
{
	class IGraphicsDevice;
	class IShaderModule;
	struct ShaderCompilationResult;

	/// @brief Describes a single uniform property within a material's uniform buffer.
	/// Tracked internally after shader reflection so that SetProperty can write
	/// to the correct offset.
	struct MaterialPropertyInfo
	{
		uint32_t offset = 0;
		uint32_t size = 0;
	};

	/// @brief Configuration options for initialising a Material3D.
	///
	/// The caller is responsible for compiling the shader and creating the
	/// IShaderModule instances beforehand.  Material3D never touches the
	/// ShaderCompiler — it only consumes already-compiled outputs.
	struct Material3DDescriptor
	{
		/// @brief Pre-compiled vertex shader module (non-owning).
		/// Must remain valid for the lifetime of the Material3D.
		IShaderModule *vertexShader = nullptr;

		/// @brief Pre-compiled fragment shader module (non-owning).
		/// Must remain valid for the lifetime of the Material3D.
		IShaderModule *fragmentShader = nullptr;

		/// @brief Vertex shader entry point name.
		std::string vertexEntry = "vertexMain";

		/// @brief Fragment shader entry point name.
		std::string fragmentEntry = "fragmentMain";

		/// @brief Compilation result that carries reflection data (bindings,
		///        vertex inputs).  The material reads this to build bind group
		///        layouts and the uniform property map.  Non-owning; must remain
		///        valid for the duration of the Init() call (it is not stored).
		const ShaderCompilationResult *compilationResult = nullptr;

		/// @brief The texture format of the color target this material will render to.
		ETextureFormat colorTargetFormat = ETextureFormat::BGRA8_UNORM;

		/// @brief Whether blending is enabled for this material's color target.
		bool blendEnabled = false;

		/// @brief Optional depth/stencil configuration.  When enabled the
		///        pipeline will be created with depth testing.
		DepthStencilState depthStencil{};

		/// @brief Optional debug name.
		std::string debugName;
	};

	/// @brief High-level material class that abstracts away the underlying
	///        graphics API and shader details.
	class Material3D
	{
	public:
		enum class EError
		{
			None,
			NullDevice,
			NullShaderModule,
			InvalidShaderModule,
			BindGroupLayoutCreationFailed,
			UniformBufferCreationFailed,
			BindGroupCreationFailed,
			PipelineCreationFailed,
			PropertyNotFound,
		};

		Material3D() = default;
		~Material3D() = default;

		Material3D(const Material3D &) = delete;
		Material3D &operator=(const Material3D &) = delete;
		Material3D(Material3D &&) noexcept = default;
		Material3D &operator=(Material3D &&) noexcept = default;

		/// @brief Create all GPU resources (pipeline, bind group layout,
		///        uniform buffer, bind group) from pre-compiled shaders.
		///
		/// The descriptor must carry valid, already-compiled IShaderModule
		/// pointers and a ShaderCompilationResult with reflection data.
		///
		/// @param device     The graphics device to create resources on.
		/// @param descriptor Material configuration.
		/// @return Success on success, or the specific EError on failure.
		std::optional<EError> Init(IGraphicsDevice *device, const Material3DDescriptor &descriptor);

		/// @brief Returns true after a successful call to Init().
		[[nodiscard]]
		bool IsInitialized() const noexcept;

		// -----------------------------------------------------------------
		// Uniform property access
		// -----------------------------------------------------------------

		/// @brief Set a uniform property by name.
		///
		/// The value is written into a CPU-side staging buffer.  Call
		/// FlushProperties() to upload the data to the GPU before rendering.
		///
		/// @tparam T  The type of the property value (must be trivially
		///            copyable and match the size declared in the shader).
		/// @param name  The property name as declared in the shader's
		///              uniform / constant buffer.
		/// @param value The value to set.
		/// @return Success if the property was found and written, or
		///         EError::PropertyNotFound if the name does not exist.
		template <class T>
		std::optional<EError> SetProperty(std::string_view name, const T &value)
		{
			auto it = m_propertyMap.find(std::string(name));
			if (it == m_propertyMap.end())
			{
				return EError::PropertyNotFound;
			}

			const MaterialPropertyInfo &info = it->second;
			const size_t writeSize = sizeof(T) < info.size ? sizeof(T) : info.size;

			if (info.offset + writeSize > m_uniformStagingBuffer.size())
			{
				return EError::PropertyNotFound;
			}

			std::memcpy(m_uniformStagingBuffer.data() + info.offset, &value, writeSize);
			m_propertiesDirty = true;
			return {};
		}

		/// @brief Upload the CPU-side uniform staging buffer to the GPU.
		///
		/// This is a no-op if no properties have been modified since the
		/// last flush (or since Init).
		///
		/// @param queue The graphics queue to submit the buffer write command on.
		void FlushProperties(IGraphicsDevice *device);

		/// @brief Convenience: set a property and immediately flush.
		template <class T>
		std::optional<EError> SetPropertyAndFlush(IGraphicsDevice *device, std::string_view name, const T &value)
		{
			auto result = SetProperty(name, value);
			if (result.has_value())
            {
                return result;
            }

			FlushProperties(device);
            return {};
		}

		/// @brief Bind this material's pipeline and bind group on the given
		///        graphics command list.
		///
		/// Call this inside a render pass, after BeginRenderPass() and before
		/// Draw() / DrawIndexed().
		///
		/// @param cmdList        The graphics command list to record on.
		/// @param bindGroupIndex The group/set index to bind the material's
		///                       bind group at (default 0).
		void Bind(IGraphicsCommandList *cmdList, uint32_t bindGroupIndex = 0) const;

		// -----------------------------------------------------------------
		// Material options
		// -----------------------------------------------------------------

		[[nodiscard]]
		EAlphaBlendMode GetAlphaBlendMode() const noexcept;

		void SetAlphaBlendMode(EAlphaBlendMode blendMode) noexcept;

		[[nodiscard]]
		ECullMode GetCullMode() const noexcept;

		void SetCullMode(ECullMode cullMode) noexcept;

		[[nodiscard]]
		EMaterialPass GetMaterialPass() const noexcept;

		void SetMaterialPass(EMaterialPass pass) noexcept;

		// -----------------------------------------------------------------
		// Internal material instance (for engine internals / renderer)
		// -----------------------------------------------------------------

		/// @brief Returns the low-level material instance holding the
		///        pipeline and bind group pointers.
		///
		/// This is used internally by the renderer when it needs to sort
		/// or batch draw calls by material.
		[[nodiscard]]
		GraphicsApiMaterialInstance *GetInternalMaterial() noexcept;

		[[nodiscard]]
		const GraphicsApiMaterialInstance *GetInternalMaterial() const noexcept;

		void SetName(std::string_view name);

		[[nodiscard]]
		const std::string &GetName() const noexcept;

		[[nodiscard]]
		IGraphicsPipeline *GetPipeline() const noexcept;

		[[nodiscard]]
		IBindGroup *GetBindGroup() const noexcept;

		[[nodiscard]]
		IBindGroupLayout *GetBindGroupLayout() const noexcept;

		[[nodiscard]]
		IGraphicsBuffer *GetUniformBuffer() const noexcept;

		/// @brief Get the reflected property map.
		[[nodiscard]]
		const std::unordered_map<std::string, MaterialPropertyInfo> &GetPropertyMap() const noexcept;

		/// @brief Get the total uniform buffer size in bytes.
		[[nodiscard]]
		uint64_t GetUniformBufferSize() const noexcept;

	private:
		/// @brief Build the property map from the shader's reflected bindings.
		void BuildPropertyMapFromReflection(const ShaderCompilationResult &result);

		/// @brief Compute the required uniform buffer size from reflected data,
		///        create the uniform buffer and bind group.
		///
		/// @return EError::None on success.
		EError CreateUniformBufferAndBindGroup(IGraphicsDevice *device,
											   std::vector<BindGroupLayoutDescriptor> &layoutDescs);

		/// @brief Create the graphics pipeline from the pre-compiled shader
		///        modules provided via the descriptor.
		///
		/// @return EError::None on success.
		EError CreatePipeline(IGraphicsDevice *device, const Material3DDescriptor &descriptor);

		/// @brief Translate EAlphaBlendMode into ColorTargetState blend fields.
		static ColorTargetState BuildColorTarget(ETextureFormat format, EAlphaBlendMode blendMode, bool blendEnabled);

		/// @brief Translate ECullMode to the RHI ECullModeFlags.
		static ECullModeFlags TranslateCullMode(ECullMode mode) noexcept;

		// -- GPU resources (owned via GraphicsResource wrappers) ---------------
		// NOTE: Shader modules are NOT owned by Material3D. The caller manages
		//       their lifetime and passes non-owning pointers through the
		//       descriptor.  Only the pipeline and binding resources are owned.

		BindGroupLayoutResource m_bindGroupLayout;
		BindGroupResource m_bindGroup;
		GraphicsPipelineResource m_pipeline;
		BufferResource m_uniformBuffer;

		/// CPU-side copy of the uniform buffer data.
		std::vector<uint8_t> m_uniformStagingBuffer;

		/// Map from property name -> offset/size within the uniform buffer.
		std::unordered_map<std::string, MaterialPropertyInfo> m_propertyMap;

		/// Whether the staging buffer has been modified since the last flush.
		bool m_propertiesDirty = false;

		EAlphaBlendMode m_alphaBlendMode = EAlphaBlendMode::None;
		ECullMode m_cullMode = ECullMode::None;
		EMaterialPass m_materialPass = EMaterialPass::MainColor;
		std::string m_name;

		/// The low-level material instance exposed to the renderer.
		GraphicsApiMaterialInstance m_internalMaterial{};

		/// Tracks whether Init() completed successfully.
		bool m_initialized = false;
	};

} // namespace Hush::Graphics
