#pragma once
#include "Shared/MaterialOptions.hpp"
#include "Shared/MaterialPass.hpp"

namespace Hush
{
	struct GraphicsApiMaterialInstance;
	class IMaterial3D
	{

	public:
		[[nodiscard]]
		virtual EAlphaBlendMode GetAlphaBlendMode() const noexcept = 0;

		virtual void SetAlphaBlendMode(EAlphaBlendMode blendMode) noexcept = 0;

		[[nodiscard]]
		virtual ECullMode GetCullMode() const noexcept = 0;

		virtual void SetCullMode(ECullMode cullMode) = 0;

		[[nodiscard]]
		virtual EMaterialPass GetMaterialPass() const noexcept = 0;
		
		virtual void SetMaterialPass(EMaterialPass pass) = 0;

		virtual GraphicsApiMaterialInstance* GetInternalMaterial() = 0;
		
	};
} // namespace Hush
