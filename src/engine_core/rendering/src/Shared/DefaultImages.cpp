#include "DefaultImages.hpp"
#include "Renderer.hpp"
#include <cstdint>

void Hush::DefaultImageProvider::CreateDefaultImages(IRenderer* renderer) {
	// Create image here for each one
	auto defaultExtent = ImageExtent3D(1U);
	constexpr Color black = Color::Black();
	constexpr Color white = Color::White();
	constexpr Color normal = { 0.5F, 0.5F, 1.0F, 1.0F };

	constexpr uint32_t vkImgUsageSampledBit = 0x00000004;
	
	uint32_t storedBlack = black.ToColor32();
	this->m_blackImage = renderer->CreateImage(&storedBlack, defaultExtent, Color::EFormat::RGBA8Unorm, vkImgUsageSampledBit);
	uint32_t storedWhite = white.ToColor32();
	
	this->m_whiteImage = renderer->CreateImage(&storedWhite, defaultExtent, Color::EFormat::RGBA8Unorm, vkImgUsageSampledBit);
	
	uint32_t storedNormal = normal.ToColor32();
	this->m_normalImage = renderer->CreateImage(&storedNormal, defaultExtent, Color::EFormat::RGBA8Unorm, vkImgUsageSampledBit);
	
	renderer->AddToDeletionQueue([this, &renderer]() {
		renderer->DestroyImage(&this->m_whiteImage);
		renderer->DestroyImage(&this->m_blackImage);
		renderer->DestroyImage(&this->m_normalImage);
			
	});
	
}
