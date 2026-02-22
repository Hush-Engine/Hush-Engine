/*! \file ISampler.hpp
	\author Alan Ramirez Herrera
	\date 2025-02-17
	\brief Abstract sampler interface and descriptor types for graphics abstraction.
*/
#pragma once

#include "GraphicsTypes.hpp"
#include <cstdint>

namespace Hush::Graphics
{
	/// @brief Texture filtering mode used for magnification, minification, and mip-level selection.
	enum class EFilterMode : uint32_t
	{
		/// @brief Nearest-neighbour (point) sampling.
		Nearest = 0,

		/// @brief Linear (bilinear) interpolation.
		Linear,
	};

	/// @brief Texture address (wrap) mode applied per UV(W) axis.
	enum class EAddressMode : uint32_t
	{
		/// @brief Tile the texture at every UV integer junction.
		Repeat = 0,

		/// @brief Tile the texture, flipping it at every integer junction.
		MirrorRepeat,

		/// @brief Clamp texture coordinates to [0, 1]; the edge texel is extended.
		ClampToEdge,
	};

	/// @brief Descriptor for creating a sampler object.
	struct SamplerDescriptor
	{
		/// @brief Magnification filter (used when the texel is larger than one pixel).
		EFilterMode magFilter = EFilterMode::Linear;

		/// @brief Minification filter (used when the texel is smaller than one pixel).
		EFilterMode minFilter = EFilterMode::Linear;

		/// @brief Mip-map filter (used when selecting between mip levels).
		EFilterMode mipmapFilter = EFilterMode::Linear;

		/// @brief Address mode for the U (S / horizontal) texture coordinate.
		EAddressMode addressModeU = EAddressMode::ClampToEdge;

		/// @brief Address mode for the V (T / vertical) texture coordinate.
		EAddressMode addressModeV = EAddressMode::ClampToEdge;

		/// @brief Address mode for the W (R / depth) texture coordinate.
		EAddressMode addressModeW = EAddressMode::ClampToEdge;

		/// @brief Minimum level-of-detail clamp value.
		float lodMinClamp = 0.0f;

		/// @brief Maximum level-of-detail clamp value.
		///
		/// Use a very large value (the default) to avoid clamping the max LOD,
		/// effectively allowing all mip levels to be used.
		float lodMaxClamp = 32.0f;

		/// @brief Comparison function for comparison (depth) samplers.
		///
		/// Set to ECompareFunction::Undefined for regular filtering samplers.
		/// Any other value creates a comparison sampler.
		ECompareFunction compare = ECompareFunction::Undefined;

		/// @brief Maximum anisotropy level (1 = no anisotropy).
		///
		/// Values > 1 enable anisotropic filtering.  The actual maximum is
		/// clamped to the device's capability.  Must be >= 1.
		uint16_t maxAnisotropy = 1;

		/// @brief Optional debug name for graphics debuggers.
		const char *debugName = nullptr;
	};

	/// @brief Returns true if the descriptor describes a comparison sampler.
	inline bool IsComparisonSampler(const SamplerDescriptor &desc)
	{
		return desc.compare != ECompareFunction::Undefined;
	}

	/// @brief Abstract sampler interface.
	///
	/// Represents a GPU sampler object that controls how textures are sampled
	/// (filtering, addressing, LOD clamping, comparison).
	///
	/// Samplers are immutable once created — create a new sampler if you need
	/// different parameters.
	class ISampler
	{
	public:
		ISampler() = default;
		virtual ~ISampler() = default;

		ISampler(const ISampler &) = delete;
		ISampler &operator=(const ISampler &) = delete;
		ISampler(ISampler &&) = delete;
		ISampler &operator=(ISampler &&) = delete;

		/// @brief Get the magnification filter mode.
		[[nodiscard]]
		virtual EFilterMode GetMagFilter() const = 0;

		/// @brief Get the minification filter mode.
		[[nodiscard]]
		virtual EFilterMode GetMinFilter() const = 0;

		/// @brief Get the mip-map filter mode.
		[[nodiscard]]
		virtual EFilterMode GetMipmapFilter() const = 0;

		/// @brief Get the address mode for the U axis.
		[[nodiscard]]
		virtual EAddressMode GetAddressModeU() const = 0;

		/// @brief Get the address mode for the V axis.
		[[nodiscard]]
		virtual EAddressMode GetAddressModeV() const = 0;

		/// @brief Get the address mode for the W axis.
		[[nodiscard]]
		virtual EAddressMode GetAddressModeW() const = 0;

		/// @brief Get the comparison function (Undefined if not a comparison sampler).
		[[nodiscard]]
		virtual ECompareFunction GetCompareFunction() const = 0;

		/// @brief Check whether this is a comparison sampler.
		[[nodiscard]]
		bool IsComparisonSampler() const
		{
			return GetCompareFunction() != ECompareFunction::Undefined;
		}

		/// @brief Get the maximum anisotropy level.
		[[nodiscard]]
		virtual uint16_t GetMaxAnisotropy() const = 0;

		/// @brief Get the native API handle.
		[[nodiscard]]
		virtual void *GetNativeHandle() const = 0;
	};

} // namespace Hush::Graphics
