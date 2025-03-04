#pragma once
#include "Shared/MaterialOptions.hpp"

namespace Hush {	
	class IMaterial3D {
		 		
	public:
        [[nodiscard]] virtual EAlphaBlendMode GetAlphaBlendMode() const noexcept = 0;

		virtual void SetAlphaBlendMode(EAlphaBlendMode blendMode) noexcept = 0;

		[[nodiscard]] virtual ECullMode GetCullMode() const noexcept = 0;
		
		virtual void SetCullMode(ECullMode cullMode) = 0;
};
}

