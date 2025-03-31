#pragma once

#include "Vector4Math.hpp"
#include <cstdint>
#include <glm/common.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace Hush {
	class Color final {
	public:

		Color() = default;
		
		Color(float red, float green, float blue) {
			this->m_rgba.r = red;
			this->m_rgba.g = green;
			this->m_rgba.b = blue;
		}
		
		Color(glm::vec3 rgb) {
			this->m_rgba = glm::vec4(rgb, 1.0F);
		}
		
		constexpr Color(glm::vec4 rgba) : m_rgba(rgba) {
			
		}
		
		[[nodiscard]] uint32_t ToColor32() const {
			constexpr float maxFloatColorValue = 255.0F;
			auto rComponent = static_cast<uint8_t>(glm::clamp(m_rgba.r, 0.0F, 1.0F) * maxFloatColorValue);
	        auto gComponent = static_cast<uint8_t>(glm::clamp(m_rgba.g, 0.0F, 1.0F) * maxFloatColorValue);
	        auto bComponent = static_cast<uint8_t>(glm::clamp(m_rgba.b, 0.0F, 1.0F) * maxFloatColorValue);
	        auto aComponent = static_cast<uint8_t>(glm::clamp(m_rgba.a, 0.0F, 1.0F) * maxFloatColorValue);

	        // Pack into a single 32-bit value as ARGB
	        return (static_cast<uint32_t>(aComponent) << 24) |
	               (static_cast<uint32_t>(rComponent) << 16) |
	               (static_cast<uint32_t>(gComponent) << 8)  |
	                static_cast<uint32_t>(bComponent);
		}

		[[nodiscard]] const glm::vec4& GetRGBA32F() const {
			return this->m_rgba;
		}
		
		glm::vec4& GetRGBA32F() {
			return this->m_rgba;
		}
		
		
	private:
		glm::vec4 m_rgba{ 0.0F, 0.0F, 0.0F, 1.0F};	
	};
}
