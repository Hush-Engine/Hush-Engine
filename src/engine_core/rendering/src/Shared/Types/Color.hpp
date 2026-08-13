#pragma once

#include <cstdint>
#include <glm/common.hpp>
#include <glm/ext/vector_float4.hpp>
#include <glm/packing.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>


#include <Hushgen.hpp>
#include <reflection/Type.hpp>
#include <serialization/Serialization.hpp>
#include <serialization/Deserialization.hpp>
#include <type_traits>

#if __has_include("Color.hushgen.hpp") && !defined(HUSH_HEADER_PARSING)
#include "Color.hushgen.hpp"
#endif

namespace Hush
{
	class [[hush::reflect]] Color final
	{

	public:
		static constexpr std ::uint64_t TypeId()
		{
			return Hush ::Hashing ::Fnv1a64(TypeName());
		}
		static constexpr std ::string_view TypeName()
		{
			return "Color";
		}
		static void RegisterReflection(Hush ::Reflection ::ReflectionDB &db)
		{
			db.RegisterClass<Hush ::Color>()
				.AddProperty(Hush ::Reflection ::FieldInfo(
					Hush ::Reflection ::GetTypeId<std ::remove_cv_t<decltype(m_rgba)>>(), "m_rgba",
					[](std ::span<const Hush ::Reflection ::VariantView> params)
						-> Hush ::Reflection ::Variant ::EVariantError {
						if (params.size() != 2)
						{
							return Hush ::Reflection ::Variant ::EVariantError ::NonSameType;
						}
						auto result = params[0].Get<Hush ::Color>();
						if (result.has_error())
						{
							return result.error();
						}
						auto *instance = result.value();
						auto value = params[1].Get<glm ::vec4>();
						if (value.has_error())
						{
							return value.error();
						}
						instance->m_rgba = *value.value();
						return {};
					},
					[](std ::span<const Hush ::Reflection ::VariantView> params)
						-> Hush ::Result<Hush ::Reflection ::Variant, Hush ::Reflection ::Variant ::EVariantError> {
						if (params.size() != 1)
						{
							return Hush ::Reflection ::Variant ::EVariantError ::NonSameType;
						}
						auto result = params[0].Get<Hush ::Color>();
						if (result.has_error())
						{
							return result.error();
						}
						auto *instance = result.value();
						return Hush ::Reflection ::Variant(instance->m_rgba);
					},
					((::size_t) & reinterpret_cast<char const volatile &>((((Hush ::Color *)0)->m_rgba)))))
				.Register();
		}
		template <typename T>
		Hush ::Serialization ::ESerializationError Serialize(T &serializer) const
		{
			auto error = serializer.template Serialize<std ::string_view>("__type", "Hush::Color");
			if (error != Hush ::Serialization ::ESerializationError ::None)
			{
				return error;
			}
			if (auto result = serializer.Serialize("m_rgba", m_rgba);
				result != Hush ::Serialization ::ESerializationError ::None)
			{
				return result;
			}
			return Hush ::Serialization ::ESerializationError ::None;
		}
		auto Deserialize(Hush ::Serialization ::IVisitor *parent, Hush ::Serialization ::EFormatDescribingType format)
		{
			struct Visitor : public Hush ::Serialization ::IVisitor
			{
				Hush ::Serialization ::Visitor<glm ::vec4> m_rgbaVisitor;
				enum class EVisitorStatus
				{
					None,
					M_RGBA,
				};
				EVisitorStatus status = EVisitorStatus ::None;
				bool insideObject{false};
				explicit Visitor(IVisitor *parent, Hush ::Color *instance,
								 Hush ::Serialization ::EFormatDescribingType format)
					: IVisitor(parent, format),
					  m_rgbaVisitor(this, &instance->m_rgba, format)
				{
					(void)instance;
					if (format == Hush ::Serialization ::EFormatDescribingType ::NonSelfDescribing)
					{
						SetStartingVisitor(&m_rgbaVisitor);
						m_rgbaVisitor.SetParentVisitor(GetParentVisitor());
					}
					else
					{
						SetStartingVisitor(this);
					}
				}
				Result VisitObjectStart() override
				{
					if (insideObject)
					{
						return Hush ::Serialization ::EDeserializationError ::InvalidData;
					}
					insideObject = true;
					return this;
				}
				Result VisitObjectEnd() override
				{
					if (!insideObject)
					{
						return Hush ::Serialization ::EDeserializationError ::InvalidData;
					}
					return GetParentVisitor();
				}
				Result VisitKey(std ::string_view value) override
				{
					(void)value;
					if (!insideObject)
					{
						return Hush ::Serialization ::EDeserializationError ::InvalidData;
					}
					if (value == "m_rgba")
					{
						status = EVisitorStatus ::M_RGBA;
						return &m_rgbaVisitor;
					}
					return this;
				}
			};
			return Visitor{parent, this, format};
		}

	private:
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

		constexpr Color(float red, float green, float blue, float alpha)
		{
			this->m_rgba.x = red;
			this->m_rgba.y = green;
			this->m_rgba.z = blue;
			this->m_rgba.w = alpha;
		}

		constexpr Color(float red, float green, float blue)
		{
			this->m_rgba.x = red;
			this->m_rgba.y = green;
			this->m_rgba.z = blue;
		}

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

		constexpr static Color Red()
		{
			return glm::vec4(1.f, 0.f, 0.f, 1.0F);
		}

		constexpr static Color WarnYellow()
		{
			// NOLINTNEXTLINE
			return glm::vec4(0.921f, 0.8f, 0.388f, 1.0f);
		}

		constexpr static Color Magenta()
		{
			return glm::vec4(1, 0, 1, 1);
		}

		constexpr static Color Transparent()
		{
			return glm::vec4(1.0F, 1.0F, 1.0F, 0.0F);
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

		[[nodiscard]]
		constexpr uint32_t ToColor32RGBA() const
		{
			constexpr float maxFloatColorValue = 255.0F;
			auto rComponent = static_cast<uint8_t>(glm::clamp(m_rgba.x, 0.0F, 1.0F) * maxFloatColorValue);
			auto gComponent = static_cast<uint8_t>(glm::clamp(m_rgba.y, 0.0F, 1.0F) * maxFloatColorValue);
			auto bComponent = static_cast<uint8_t>(glm::clamp(m_rgba.z, 0.0F, 1.0F) * maxFloatColorValue);
			auto aComponent = static_cast<uint8_t>(glm::clamp(m_rgba.w, 0.0F, 1.0F) * maxFloatColorValue);

			return (static_cast<uint32_t>(rComponent) << 24) | (static_cast<uint32_t>(gComponent) << 16) |
				   (static_cast<uint32_t>(bComponent) << 8) | static_cast<uint32_t>(aComponent);
		}

		[[nodiscard]]
		constexpr uint32_t ToColor32ABGR() const
		{
			constexpr float maxFloatColorValue = 255.0F;
			auto rComponent = static_cast<uint8_t>(glm::clamp(m_rgba.x, 0.0F, 1.0F) * maxFloatColorValue);
			auto gComponent = static_cast<uint8_t>(glm::clamp(m_rgba.y, 0.0F, 1.0F) * maxFloatColorValue);
			auto bComponent = static_cast<uint8_t>(glm::clamp(m_rgba.z, 0.0F, 1.0F) * maxFloatColorValue);
			auto aComponent = static_cast<uint8_t>(glm::clamp(m_rgba.w, 0.0F, 1.0F) * maxFloatColorValue);

			return (static_cast<uint32_t>(aComponent) << 24) | (static_cast<uint32_t>(bComponent) << 16) |
				   (static_cast<uint32_t>(gComponent) << 8) | static_cast<uint32_t>(rComponent);
		}

	private:
		[[hush::property]]
		glm::vec4 m_rgba{0.0F, 0.0F, 0.0F, 1.0F};
	};
} // namespace Hush
