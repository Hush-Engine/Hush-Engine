#pragma once

#include "Shared/GpuAllocatedImage.hpp"
namespace Hush {

	class IRenderer;

	class DefaultImageProvider {
	public:
		void CreateDefaultImages(IRenderer* renderer);

		[[nodiscard]]
		const GpuAllocatedImage& GetWhiteImage() const {
			return this->m_whiteImage;
		}
		
		[[nodiscard]]
		const GpuAllocatedImage& GetBlackImage() const {
			return this->m_blackImage;
		}
		
		[[nodiscard]]
		const GpuAllocatedImage& GetNormalImage() const {
			return this->m_normalImage;
		}

	private:
		GpuAllocatedImage m_whiteImage {};
		
		GpuAllocatedImage m_blackImage {};
		
		GpuAllocatedImage m_normalImage {};
		
	};
	
}

