#pragma once
#include "Shared/MaterialOptions.hpp"
#include "Shared/MaterialPass.hpp"
#include <string_view>

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

		virtual GraphicsApiMaterialInstance *GetInternalMaterial() = 0;

		virtual void SetName(const std::string_view& name) = 0;
		
		[[nodiscard]] virtual std::string_view GetName() const noexcept = 0;
	};

	void Serialize(IMaterial3D *component, const char* uniqueName);

} // namespace Hush
