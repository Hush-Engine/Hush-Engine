#pragma once
#include <cstring>
#include <filesystem>
#include <span>
#include <unordered_map>
#include "ShaderBindings.hpp"
#include "Result.hpp"
#include "Assertions.hpp"
#include "Shared/IMaterial3D.hpp"
#include "Shared/MaterialOptions.hpp"
#include "Shared/Types/MaterialInstance.hpp"

class SpvReflectTypeDescription;

namespace Hush
{
	struct OpaqueMaterialData;
#if defined(HUSH_VULKAN_IMPL)
	struct DescriptorAllocatorGrowable;
	class VulkanAllocatedBuffer;
	using OpaqueDescriptorAllocator = DescriptorAllocatorGrowable;
#endif

	class IRenderer;
	/// @brief Represents a material created by a custom shader with dynamic mappings and reflections
	/// The performance impact of this class is considerable since it needs to keep track of the bindings
	/// in both RAM and GPU, as well as process the shader initially with Reflection (initialization cost)
	/// This class's interface is Rendering API agnostic
	class ShaderMaterial final : public IMaterial3D
	{
	public:
		enum class EError
		{
			None = 0,
			FragmentShaderNotFound,
			VertexShaderNotFound,
			ReflectionError,
			PipelineLayoutCreationFailed,
			PropertyNotFound,
			ShaderNotLoaded
		};

		ShaderMaterial() = default;

		~ShaderMaterial();

		/// @brief Will create and bind pipelines for both shaders
		// Returns an error in case this fails (not a result because the underlying type is void)
		EError LoadShaders(IRenderer *renderer, const std::filesystem::path &fragmentShaderPath,
						   const std::filesystem::path &vertexShaderPath);

		void GenerateMaterialInstance(OpaqueDescriptorAllocator *descriptorAllocator);

		OpaqueMaterialData *GetMaterialData();

		[[nodiscard]]
		EAlphaBlendMode GetAlphaBlendMode() const noexcept override;

		void SetAlphaBlendMode(EAlphaBlendMode blendMode) noexcept override;

		[[nodiscard]]
		ECullMode GetCullMode() const noexcept override;

		void SetCullMode(ECullMode cullMode) override;

	
		[[nodiscard]]
		EMaterialPass GetMaterialPass() const noexcept override;
		
		void SetMaterialPass(EMaterialPass pass) override;
		
		template <class T>
		inline EError SetProperty(const std::string_view &name, T value)
		{
			// Search for a binding with the name passed onto the func
			constexpr size_t valueSize = sizeof(T);
			const ShaderBindings &binding = this->FindBinding(name);
			if (this->m_bindingsByName.find(name.data()) == this->m_bindingsByName.end())
			{
				return EError::PropertyNotFound;
			}
			HUSH_ASSERT(this->m_uniformBufferMappedData != nullptr,
						"Material buffer is not initialized! Forgot to call LoadShaders?");
			// Offset the pointer by the binding's offset
			std::byte *dataStartingPoint = static_cast<std::byte *>(this->m_uniformBufferMappedData) + binding.offset;
			// Memcpy the data with sizeof(T)
			memcpy(dataStartingPoint, &value, valueSize);
			return EError::None;
		}

		template <class T>
		inline Result<T, EError> GetProperty(const std::string_view &name)
		{
			HUSH_COND_FAIL_V(this->m_bindingsByName.find(name.data()) != this->m_bindingsByName.end(),
							 EError::PropertyNotFound);
			// Search for a binding with the name passed onto the func
			const ShaderBindings &binding = this->FindBinding(name);

			if (this->m_uniformBufferMappedData == nullptr)
			{
				return EError::ShaderNotLoaded;
			}
			std::byte *dataStartingPoint = static_cast<std::byte *>(this->m_uniformBufferMappedData) + binding.offset;

			// Important to reinterpret cast using T, because some stuff might be 16byte-aligned and we want to only get
			// the bytes That correspond to the actual value type
			return *reinterpret_cast<T *>(dataStartingPoint);
		}

		GraphicsApiMaterialInstance* GetInternalMaterial() override;

	private:
		Result<std::vector<ShaderBindings>, EError> ReflectShader(const std::span<std::uint32_t> &shaderBinary);

		uint32_t GetAPIBinding(ShaderBindings::EBindingType agnosticBinding);

		EError BindShader(const std::vector<ShaderBindings> &vertBindings,
						  const std::vector<ShaderBindings> &fragBindings);

		void InitializeMaterialDataMembers();

		size_t CalculateTypeSize(const SpvReflectTypeDescription *type);

		const ShaderBindings &FindBinding(const std::string_view &name);

		IRenderer *m_renderer;
		OpaqueMaterialData *m_materialData;

		std::unordered_map<std::string, ShaderBindings> m_bindingsByName;

		std::unique_ptr<GraphicsApiMaterialInstance> m_internalMaterial;

		size_t m_uniformBufferSize;

		void *m_uniformBufferMappedData = nullptr;

		EAlphaBlendMode m_alphaBlendMode = EAlphaBlendMode::None;

		ECullMode m_cullMode = ECullMode::None;
	};
} // namespace Hush
