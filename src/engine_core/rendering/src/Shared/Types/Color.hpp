#pragma once

#include <cstdint>
#include <glm/common.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/packing.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace Hush
{
	class Color final
	{
	public:
		enum class EFormat : uint32_t
		{
			Unknown,
			R8Unorm,
			RG8Unorm,
			RGB8Unorm,
			RGBA8Unorm,
			R16Float,
			RG16Float,
			RGB16Float,
			RGBA16Float,
			R32Float,
			RG32Float,
			RGB32Float,
			RGBA32Float,
			D32Float,
			D24S8Unorm,
		};

		Color() = default;

		/**
		 * @brief Constructs a Color with specified red, green, blue, and alpha components.
		 *
		 * @param red Red component in the range [0, 1].
		 * @param green Green component in the range [0, 1].
		 * @param blue Blue component in the range [0, 1].
		 * @param alpha Alpha (opacity) component in the range [0, 1].
		 */
		constexpr Color(float red, float green, float blue, float alpha)
		{
			this->m_rgba.x = red;
			this->m_rgba.y = green;
			this->m_rgba.z = blue;
			this->m_rgba.w = alpha;
		}

		/**
		 * @brief Constructs a color from red, green, and blue components with alpha set to 1.0.
		 *
		 * @param red Red component in the range [0.0, 1.0].
		 * @param green Green component in the range [0.0, 1.0].
		 * @param blue Blue component in the range [0.0, 1.0].
		 */
		constexpr Color(float red, float green, float blue)
		{
			this->m_rgba.x = red;
			this->m_rgba.y = green;
			this->m_rgba.z = blue;
		}

		/**
		 * @brief Constructs a Color from a 3-component RGB vector with alpha set to 1.0.
		 *
		 * @param rgb The RGB color components as a glm::vec3.
		 */
		constexpr Color(glm::vec3 rgb)
		{
			this->m_rgba = glm::vec4(rgb, 1.0F);
		}

		constexpr Color(glm::vec4 rgba)
			: m_rgba(rgba)
		{
		}

		constexpr static Color White()
		{
			return glm::vec4(1.0F);
		}

		constexpr static Color Black()
		{
			return glm::vec4(0.0F);
		}

		constexpr static Color Magenta()
		{
			return glm::vec4(1, 0, 1, 1);
		}

		[[nodiscard]]
		constexpr const glm::vec4 &GetRGBA32F() const
		{
			return this->m_rgba;
		}

		glm::vec4 &GetRGBA32F()
		{
			return this->m_rgba;
		}

		/**
		 * @brief Converts the color to a 32-bit packed ARGB integer.
		 *
		 * Clamps each RGBA component to [0, 1], scales to 8 bits, and packs them into a single uint32_t in ARGB order (alpha in the highest 8 bits, red, green, blue in decreasing order).
		 *
		 * @return uint32_t The color represented as a packed 32-bit ARGB value.
		 */
		[[nodiscard]]
		constexpr uint32_t ToColor32() const
		{
			constexpr float maxFloatColorValue = 255.0F;
			auto rComponent = static_cast<uint8_t>(glm::clamp(m_rgba.x, 0.0F, 1.0F) * maxFloatColorValue);
			auto gComponent = static_cast<uint8_t>(glm::clamp(m_rgba.y, 0.0F, 1.0F) * maxFloatColorValue);
			auto bComponent = static_cast<uint8_t>(glm::clamp(m_rgba.z, 0.0F, 1.0F) * maxFloatColorValue);
			auto aComponent = static_cast<uint8_t>(glm::clamp(m_rgba.w, 0.0F, 1.0F) * maxFloatColorValue);

			// Pack into a single 32-bit value as ARGB
			return (static_cast<uint32_t>(aComponent) << 24) | (static_cast<uint32_t>(rComponent) << 16) |
				   (static_cast<uint32_t>(gComponent) << 8) | static_cast<uint32_t>(bComponent);
		}

	private:
		glm::vec4 m_rgba{0.0F, 0.0F, 0.0F, 1.0F};
	};
} // namespace Hush
