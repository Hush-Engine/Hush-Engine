/*! \file IBindGroup.hpp
	\author Alan Ramirez Herrera
	\date 2026-02-17
	\brief Bind group and bind group layout interfaces for GPU resource binding.
*/
#pragma once

#include "GraphicsTypes.hpp"
#include "IShaderModule.hpp"
#include <cstdint>
#include <string>
#include <vector>

namespace Hush::Graphics
{
	// Forward declarations
	class IGraphicsBuffer;
	class IGraphicsTexture;
	class IBindGroupLayout;
	class ISampler;

	/// @brief Bitmask flags indicating which shader stages can access a binding.
	enum class EShaderStageFlags : uint32_t
	{
		None = 0,
		Vertex = 1 << 0,
		Fragment = 1 << 1,
		Compute = 1 << 2,
		All = Vertex | Fragment | Compute,
	};

	inline EShaderStageFlags operator|(EShaderStageFlags a, EShaderStageFlags b)
	{
		return static_cast<EShaderStageFlags>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
	}

	inline EShaderStageFlags operator&(EShaderStageFlags a, EShaderStageFlags b)
	{
		return static_cast<EShaderStageFlags>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
	}

	inline EShaderStageFlags &operator|=(EShaderStageFlags &a, EShaderStageFlags b)
	{
		a = a | b;
		return a;
	}

	inline bool HasStageFlag(EShaderStageFlags flags, EShaderStageFlags test)
	{
		return (static_cast<uint32_t>(flags) & static_cast<uint32_t>(test)) != 0;
	}

	/// @brief The type of resource bound at a particular binding slot.
	enum class EBindingType : uint32_t
	{
		/// @brief Uniform / constant buffer (read-only, small, frequently updated)
		UniformBuffer = 0,

		/// @brief Storage buffer (read-write, large, compute-friendly)
		StorageBuffer,

		/// @brief Read-only storage buffer
		ReadOnlyStorageBuffer,

		/// @brief Sampled texture (used with a sampler for filtered reads)
		SampledTexture,

		/// @brief Storage texture (unfiltered read/write, typically for compute)
		StorageTexture,

		/// @brief Sampler object (filtering, addressing modes)
		Sampler,

		/// @brief Comparison sampler (for shadow mapping, etc.)
		ComparisonSampler,
	};

	/// @brief The component type that a sampled texture binding expects.
	enum class ETextureSampleType : uint32_t
	{
		Float = 0,		   ///< Filterable float texture
		UnfilterableFloat, ///< Non-filterable float texture
		Depth,			   ///< Depth comparison texture
		Sint,			   ///< Signed integer texture
		Uint,			   ///< Unsigned integer texture
	};

	/// @brief Access mode for storage texture bindings.
	enum class EStorageTextureAccess : uint32_t
	{
		WriteOnly = 0,
		ReadOnly,
		ReadWrite,
	};

	/// @brief Sampler type classification for layout entries.
	enum class ESamplerBindingType : uint32_t
	{
		Filtering = 0, ///< Standard filtering sampler
		NonFiltering,  ///< Non-filtering (nearest) sampler
		Comparison,	   ///< Comparison sampler (for shadow maps)
	};

	/// @brief Describes a single binding within a bind group layout.
	///
	/// Each entry declares the binding index, the type of resource expected,
	/// which shader stages can see it, and type-specific configuration.
	struct BindGroupLayoutEntry
	{
		/// @brief Binding index (corresponds to @binding(N) in WGSL / binding in
		///        Slang / layout(binding = N) in GLSL).
		uint32_t binding = 0;

		/// @brief The kind of resource at this binding slot
		EBindingType type = EBindingType::UniformBuffer;

		/// @brief Which shader stages this binding is visible to
		EShaderStageFlags stageFlags = EShaderStageFlags::All;

		/// @brief For UniformBuffer / StorageBuffer / ReadOnlyStorageBuffer:
		///        whether the buffer binding has a dynamic offset.
		bool hasDynamicOffset = false;

		/// @brief For UniformBuffer / StorageBuffer:
		///        minimum buffer binding size (0 = no constraint).
		uint64_t minBufferBindingSize = 0;

		/// @brief For SampledTexture: the expected sample type.
		ETextureSampleType textureSampleType = ETextureSampleType::Float;

		/// @brief For SampledTexture / StorageTexture: the texture view dimension.
		/// 0 = undefined, 1 = 1D, 2 = 2D, 3 = 3D, 4 = Cube, 5 = 2DArray, etc.
		/// (Backends translate this to their own enum.)
		uint32_t textureViewDimension = 2; // default: 2D

		/// @brief For SampledTexture: whether the texture is multisampled.
		bool textureMultisampled = false;

		/// @brief For StorageTexture: the access mode.
		EStorageTextureAccess storageTextureAccess = EStorageTextureAccess::WriteOnly;

		/// @brief For StorageTexture: the texture format.
		ETextureFormat storageTextureFormat = ETextureFormat::RGBA8_UNORM;

		/// @brief For Sampler / ComparisonSampler: the sampler binding type.
		ESamplerBindingType samplerType = ESamplerBindingType::Filtering;
	};

	/// @brief Descriptor for creating a bind group layout.
	struct BindGroupLayoutDescriptor
	{
		/// @brief The entries that make up this layout.
		std::vector<BindGroupLayoutEntry> entries;

		/// @brief Optional debug name.
		std::string debugName;
	};

	/// @brief Abstract interface for a bind group layout.
	///
	/// A bind group layout declares the expected shape (binding indices, types,
	/// stage visibility) of a set of resource bindings.  It is used when creating
	/// both pipelines (for validation and driver optimisation) and bind groups
	/// (as the template that the group must match).
	class IBindGroupLayout
	{
	public:
		IBindGroupLayout() = default;
		virtual ~IBindGroupLayout() = default;

		IBindGroupLayout(const IBindGroupLayout &) = delete;
		IBindGroupLayout &operator=(const IBindGroupLayout &) = delete;
		IBindGroupLayout(IBindGroupLayout &&) = delete;
		IBindGroupLayout &operator=(IBindGroupLayout &&) = delete;

		/// @brief Get the number of entries in this layout
		[[nodiscard]]
		virtual uint32_t GetEntryCount() const = 0;

		/// @brief Check if the layout is valid
		[[nodiscard]]
		virtual bool IsValid() const = 0;

		/// @brief Get the native API handle
		[[nodiscard]]
		virtual void *GetNativeHandle() const = 0;
	};

	/// @brief Describes a single resource binding within a bind group.
	///
	/// Exactly one of the resource pointer fields (buffer, texture, sampler)
	/// should be non-null, matching the type declared in the corresponding
	/// BindGroupLayoutEntry.
	struct BindGroupEntry
	{
		/// @brief Binding index (must match a BindGroupLayoutEntry::binding)
		uint32_t binding = 0;

		// -- Buffer binding -------------------------------------------------

		/// @brief Buffer to bind (for UniformBuffer / StorageBuffer /
		///        ReadOnlyStorageBuffer entries).
		IGraphicsBuffer *buffer = nullptr;

		/// @brief Byte offset into the buffer
		uint64_t offset = 0;

		/// @brief Number of bytes to expose to the shader.
		/// Use 0 or UINT64_MAX for "whole buffer from offset".
		uint64_t size = 0;

		// -- Texture binding ------------------------------------------------

		/// @brief Texture to bind (for SampledTexture / StorageTexture entries).
		IGraphicsTexture *texture = nullptr;

		// -- Sampler binding ------------------------------------------------

		/// @brief Sampler to bind (for Sampler / ComparisonSampler entries).
		ISampler *sampler = nullptr;
	};

	/// @brief Descriptor for creating a bind group.
	struct BindGroupDescriptor
	{
		/// @brief The layout this bind group conforms to.
		IBindGroupLayout *layout = nullptr;

		/// @brief The resource bindings.
		std::vector<BindGroupEntry> entries;

		/// @brief Optional debug name.
		std::string debugName;
	};

	/// @brief Abstract interface for a bind group (set of resource bindings).
	class IBindGroup
	{
	public:
		IBindGroup() = default;
		virtual ~IBindGroup() = default;

		IBindGroup(const IBindGroup &) = delete;
		IBindGroup &operator=(const IBindGroup &) = delete;
		IBindGroup(IBindGroup &&) = delete;
		IBindGroup &operator=(IBindGroup &&) = delete;

		/// @brief Get the layout this bind group was created from
		[[nodiscard]]
		virtual IBindGroupLayout *GetLayout() const = 0;

		/// @brief Check if the bind group is valid
		[[nodiscard]]
		virtual bool IsValid() const = 0;

		/// @brief Get the native API handle
		/// For WebGPU: wgpu::BindGroup*
		/// For Vulkan: VkDescriptorSet*
		[[nodiscard]]
		virtual void *GetNativeHandle() const = 0;
	};

} // namespace Hush::Graphics
