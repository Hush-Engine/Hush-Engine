#pragma once

#include "Shared/MaterialOptions.hpp"
#include "Shared/MaterialPass.hpp"
#include <string>
#include <string_view>

namespace Hush
{
	namespace Graphics
	{
		struct GraphicsApiMaterialInstance;
	}

	/// @brief Abstract interface for all material types in the engine.
	///
	/// This is the base class that the mesh system (GeoSurface) stores via
	/// std::shared_ptr<IMaterial3D>.  Concrete implementations include:
	///   - ShaderMaterial  (legacy Vulkan-specific, SPIR-V reflection based)
	///   - Graphics::Material3D (new RHI-agnostic, Slang/ShaderCompiler based)
	///   - GLTFMetallicRoughness (PBR material loaded from glTF assets)
	class IMaterial3D
	{
	public:
		IMaterial3D() = default;
		virtual ~IMaterial3D() = default;

		IMaterial3D(const IMaterial3D &) = default;
		IMaterial3D &operator=(const IMaterial3D &) = default;
		IMaterial3D(IMaterial3D &&) = default;
		IMaterial3D &operator=(IMaterial3D &&) = default;

		[[nodiscard]]
		virtual EAlphaBlendMode GetAlphaBlendMode() const noexcept = 0;

		virtual void SetAlphaBlendMode(EAlphaBlendMode blendMode) noexcept = 0;

		[[nodiscard]]
		virtual ECullMode GetCullMode() const noexcept = 0;

		virtual void SetCullMode(ECullMode cullMode) = 0;

		[[nodiscard]]
		virtual EMaterialPass GetMaterialPass() const noexcept = 0;

		virtual void SetMaterialPass(EMaterialPass pass) = 0;

		/// @brief Returns the low-level, graphics-API-specific material instance
		///        that holds the pipeline and bind group (or descriptor set)
		///        pointers needed for rendering.
		///
		/// Implementations that do not use the RHI MaterialInstance path may
		/// return nullptr.
		virtual Graphics::GraphicsApiMaterialInstance *GetInternalMaterial() = 0;

		virtual void SetName(const std::string_view &name) = 0;

		[[nodiscard]]
		virtual const std::string &GetName() const noexcept = 0;
	};

} // namespace Hush