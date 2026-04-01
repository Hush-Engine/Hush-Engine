/*! \file WebGPUBindGroup.hpp
	\author Alan Ramirez Herrera
	\date 2026-02-17
	\brief WebGPU implementations of IBindGroupLayout and IBindGroup.

	These classes wrap wgpu::BindGroupLayout and wgpu::BindGroup respectively,
	translating the engine's backend-agnostic bind group descriptors into
	WebGPU-native structures.

	Bind group layouts describe the shape of a set of resource bindings and
	are used at pipeline creation time. Bind groups are concrete instances
	that hold actual resource handles and are set on command lists before
	draw / dispatch calls.
*/
#pragma once

#include "../RHI/IBindGroup.hpp"
#include <webgpu/webgpu.hpp>

namespace Hush::Graphics
{
	// Forward declarations for use in bind group entry translation
	class WebGPUBuffer;
	class WebGPUTexture;

	/// @brief Convert engine shader stage flags to wgpu::ShaderStage.
	inline wgpu::ShaderStage ConvertShaderStageFlags(EShaderStageFlags flags)
	{
		WGPUShaderStage result = WGPUShaderStage_None;
		if (HasStageFlag(flags, EShaderStageFlags::Vertex))
		{
			result |= WGPUShaderStage_Vertex;
		}
		if (HasStageFlag(flags, EShaderStageFlags::Fragment))
		{
			result |= WGPUShaderStage_Fragment;
		}
		if (HasStageFlag(flags, EShaderStageFlags::Compute))
		{
			result |= WGPUShaderStage_Compute;
		}
		return static_cast<wgpu::ShaderStage>(result);
	}

	/// @brief Convert engine texture view dimension to wgpu::TextureViewDimension.
	inline wgpu::TextureViewDimension ConvertTextureViewDimension(uint32_t dimension)
	{
		switch (dimension)
		{
		case 1:
			return wgpu::TextureViewDimension::_1D;
		case 2:
			return wgpu::TextureViewDimension::_2D;
		case 3:
			return wgpu::TextureViewDimension::_3D;
		case 4:
			return wgpu::TextureViewDimension::Cube;
		case 5:
			return wgpu::TextureViewDimension::_2DArray;
		case 6:
			return wgpu::TextureViewDimension::CubeArray;
		default:
			return wgpu::TextureViewDimension::_2D;
		}
	}

	/// @brief Convert engine texture sample type to wgpu::TextureSampleType.
	inline wgpu::TextureSampleType ConvertTextureSampleType(ETextureSampleType sampleType)
	{
		switch (sampleType)
		{
		case ETextureSampleType::Float:
			return wgpu::TextureSampleType::Float;
		case ETextureSampleType::UnfilterableFloat:
			return wgpu::TextureSampleType::UnfilterableFloat;
		case ETextureSampleType::Depth:
			return wgpu::TextureSampleType::Depth;
		case ETextureSampleType::Sint:
			return wgpu::TextureSampleType::Sint;
		case ETextureSampleType::Uint:
			return wgpu::TextureSampleType::Uint;
		default:
			return wgpu::TextureSampleType::Float;
		}
	}

	/// @brief Convert engine storage texture access to wgpu::StorageTextureAccess.
	inline wgpu::StorageTextureAccess ConvertStorageTextureAccess(EStorageTextureAccess access)
	{
		switch (access)
		{
		case EStorageTextureAccess::WriteOnly:
			return wgpu::StorageTextureAccess::WriteOnly;
		case EStorageTextureAccess::ReadOnly:
			return wgpu::StorageTextureAccess::ReadOnly;
		case EStorageTextureAccess::ReadWrite:
			return wgpu::StorageTextureAccess::ReadWrite;
		default:
			return wgpu::StorageTextureAccess::WriteOnly;
		}
	}

	/// @brief Convert engine sampler binding type to wgpu::SamplerBindingType.
	inline wgpu::SamplerBindingType ConvertSamplerBindingType(ESamplerBindingType type)
	{
		switch (type)
		{
		case ESamplerBindingType::Filtering:
			return wgpu::SamplerBindingType::Filtering;
		case ESamplerBindingType::NonFiltering:
			return wgpu::SamplerBindingType::NonFiltering;
		case ESamplerBindingType::Comparison:
			return wgpu::SamplerBindingType::Comparison;
		default:
			return wgpu::SamplerBindingType::Filtering;
		}
	}

	/// @brief Convert engine texture format to wgpu::TextureFormat (for bind group / storage texture usage).
	inline wgpu::TextureFormat ConvertTextureFormatForBindGroup(ETextureFormat format)
	{
		switch (format)
		{
		case ETextureFormat::R8_UNORM:
			return wgpu::TextureFormat::R8Unorm;
		case ETextureFormat::R8_SNORM:
			return wgpu::TextureFormat::R8Snorm;
		case ETextureFormat::R8_UINT:
			return wgpu::TextureFormat::R8Uint;
		case ETextureFormat::R8_SINT:
			return wgpu::TextureFormat::R8Sint;
		case ETextureFormat::R16_UINT:
			return wgpu::TextureFormat::R16Uint;
		case ETextureFormat::R16_SINT:
			return wgpu::TextureFormat::R16Sint;
		case ETextureFormat::R16_FLOAT:
			return wgpu::TextureFormat::R16Float;
		case ETextureFormat::R32_UINT:
			return wgpu::TextureFormat::R32Uint;
		case ETextureFormat::R32_SINT:
			return wgpu::TextureFormat::R32Sint;
		case ETextureFormat::R32_FLOAT:
			return wgpu::TextureFormat::R32Float;
		case ETextureFormat::RG8_UNORM:
			return wgpu::TextureFormat::RG8Unorm;
		case ETextureFormat::RG8_SNORM:
			return wgpu::TextureFormat::RG8Snorm;
		case ETextureFormat::RG16_FLOAT:
			return wgpu::TextureFormat::RG16Float;
		case ETextureFormat::RG32_FLOAT:
			return wgpu::TextureFormat::RG32Float;
		case ETextureFormat::RGBA8_UNORM:
			return wgpu::TextureFormat::RGBA8Unorm;
		case ETextureFormat::RGBA8_SRGB:
			return wgpu::TextureFormat::RGBA8UnormSrgb;
		case ETextureFormat::RGBA16_FLOAT:
			return wgpu::TextureFormat::RGBA16Float;
		case ETextureFormat::RGBA32_FLOAT:
			return wgpu::TextureFormat::RGBA32Float;
		case ETextureFormat::BGRA8_UNORM:
			return wgpu::TextureFormat::BGRA8Unorm;
		case ETextureFormat::BGRA8_SRGB:
			return wgpu::TextureFormat::BGRA8UnormSrgb;
		default:
			return wgpu::TextureFormat::RGBA8Unorm;
		}
	}

	/// @brief WebGPU implementation of IBindGroupLayout.
	///
	/// Wraps a wgpu::BindGroupLayout that describes the expected shape (binding
	/// indices, types, shader stage visibility) of a set of resource bindings.
	/// Used at pipeline creation time for validation and at bind group creation
	/// time as the template the group must match.
	class WebGPUBindGroupLayout : public IBindGroupLayout
	{
	public:
		/// @brief Construct a bind group layout from the engine descriptor.
		///
		/// Translates the descriptor's entries into wgpu::BindGroupLayoutEntry
		/// structures and creates the native wgpu::BindGroupLayout.
		///
		/// @param device     The WebGPU device.
		/// @param descriptor The backend-agnostic bind group layout descriptor.
		WebGPUBindGroupLayout(wgpu::Device device, const BindGroupLayoutDescriptor &descriptor);

		~WebGPUBindGroupLayout() override;

		// Non-copyable, non-movable
		WebGPUBindGroupLayout(const WebGPUBindGroupLayout &) = delete;
		WebGPUBindGroupLayout &operator=(const WebGPUBindGroupLayout &) = delete;
		WebGPUBindGroupLayout(WebGPUBindGroupLayout &&) = delete;
		WebGPUBindGroupLayout &operator=(WebGPUBindGroupLayout &&) = delete;

		[[nodiscard]]
		uint32_t GetEntryCount() const override
		{
			return m_entryCount;
		}

		[[nodiscard]]
		bool IsValid() const override
		{
			return m_layout != nullptr;
		}

		[[nodiscard]]
		void *GetNativeHandle() const override
		{
			// Return the raw WGPUBindGroupLayout handle as void*.
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
			return reinterpret_cast<void *>(static_cast<WGPUBindGroupLayout>(m_layout));
		}

		/// @brief Get the underlying wgpu::BindGroupLayout directly.
		[[nodiscard]]
		wgpu::BindGroupLayout GetLayout() const
		{
			return m_layout;
		}

		/// @brief Get the debug name assigned at creation time.
		[[nodiscard]]
		const std::string &GetDebugName() const
		{
			return m_debugName;
		}

	private:
		/// @brief The compiled WebGPU bind group layout.
		wgpu::BindGroupLayout m_layout = nullptr;

		/// @brief Number of entries in this layout.
		uint32_t m_entryCount = 0;

		/// @brief Optional debug name.
		std::string m_debugName;
	};

	/// @brief WebGPU implementation of IBindGroup.
	///
	/// Wraps a wgpu::BindGroup that holds concrete resource bindings (buffers,
	/// textures, samplers) matching a bind group layout.  Bind groups are set
	/// on a command list before draw / dispatch calls via SetBindGroup().
	class WebGPUBindGroup : public IBindGroup
	{
	public:
		/// @brief Construct a bind group from the engine descriptor.
		///
		/// Translates the descriptor's entries into wgpu::BindGroupEntry
		/// structures and creates the native wgpu::BindGroup.
		///
		/// @param device     The WebGPU device.
		/// @param descriptor The backend-agnostic bind group descriptor.
		WebGPUBindGroup(wgpu::Device device, const BindGroupDescriptor &descriptor);

		~WebGPUBindGroup() override;

		// Non-copyable, non-movable
		WebGPUBindGroup(const WebGPUBindGroup &) = delete;
		WebGPUBindGroup &operator=(const WebGPUBindGroup &) = delete;
		WebGPUBindGroup(WebGPUBindGroup &&) = delete;
		WebGPUBindGroup &operator=(WebGPUBindGroup &&) = delete;

		[[nodiscard]]
		IBindGroupLayout *GetLayout() const override
		{
			return m_layout;
		}

		[[nodiscard]]
		bool IsValid() const override
		{
			return m_bindGroup != nullptr;
		}

		[[nodiscard]]
		void *GetNativeHandle() const override
		{
			// Return the raw WGPUBindGroup handle as void*.
			// NOLINTNEXTLINE(cppcoreguidelines-pro-type-reinterpret-cast)
			return reinterpret_cast<void *>(static_cast<WGPUBindGroup>(m_bindGroup));
		}

		/// @brief Get the underlying wgpu::BindGroup directly.
		[[nodiscard]]
		wgpu::BindGroup GetBindGroup() const
		{
			return m_bindGroup;
		}

		/// @brief Get the debug name assigned at creation time.
		[[nodiscard]]
		const std::string &GetDebugName() const
		{
			return m_debugName;
		}

	private:
		/// @brief The compiled WebGPU bind group.
		wgpu::BindGroup m_bindGroup = nullptr;

		/// @brief The layout this bind group was created from (non-owning).
		IBindGroupLayout *m_layout = nullptr;

		/// @brief Optional debug name.
		std::string m_debugName;
	};

} // namespace Hush::Graphics
