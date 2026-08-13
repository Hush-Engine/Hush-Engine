#pragma once

#include "Types/Color.hpp"
#include "Vector4Math.hpp"
#include <string_view>


#include <Hushgen.hpp>
#include <reflection/Type.hpp>
#include <serialization/Serialization.hpp>
#include <serialization/Deserialization.hpp>
#include <type_traits>

#if __has_include("DirectionalLight.hushgen.hpp") && !defined(HUSH_HEADER_PARSING)
#include "DirectionalLight.hushgen.hpp"
#endif

namespace Hush
{
	struct [[hush::reflect]] DirectionalLight
	{

	public:
		static constexpr std ::uint64_t TypeId()
		{
			return Hush ::Hashing ::Fnv1a64(TypeName());
		}
		static constexpr std ::string_view TypeName()
		{
			return "DirectionalLight";
		}
		static void RegisterReflection(Hush ::Reflection ::ReflectionDB &db)
		{
			db.RegisterClass<Hush ::DirectionalLight>()
				.AddProperty(Hush ::Reflection ::FieldInfo(
					Hush ::Reflection ::GetTypeId<std ::remove_cv_t<decltype(intensity)>>(), "intensity",
					[](std ::span<const Hush ::Reflection ::VariantView> params)
						-> Hush ::Reflection ::Variant ::EVariantError {
						if (params.size() != 2)
						{
							return Hush ::Reflection ::Variant ::EVariantError ::NonSameType;
						}
						auto result = params[0].Get<Hush ::DirectionalLight>();
						if (result.has_error())
						{
							return result.error();
						}
						auto *instance = result.value();
						auto value = params[1].Get<float>();
						if (value.has_error())
						{
							return value.error();
						}
						instance->intensity = *value.value();
						return {};
					},
					[](std ::span<const Hush ::Reflection ::VariantView> params)
						-> Hush ::Result<Hush ::Reflection ::Variant, Hush ::Reflection ::Variant ::EVariantError> {
						if (params.size() != 1)
						{
							return Hush ::Reflection ::Variant ::EVariantError ::NonSameType;
						}
						auto result = params[0].Get<Hush ::DirectionalLight>();
						if (result.has_error())
						{
							return result.error();
						}
						auto *instance = result.value();
						return Hush ::Reflection ::Variant(instance->intensity);
					},
					((::size_t) &
					 reinterpret_cast<char const volatile &>((((Hush ::DirectionalLight *)0)->intensity)))))
				.AddProperty(Hush ::Reflection ::FieldInfo(
					Hush ::Reflection ::GetTypeId<std ::remove_cv_t<decltype(color)>>(), "color",
					[](std ::span<const Hush ::Reflection ::VariantView> params)
						-> Hush ::Reflection ::Variant ::EVariantError {
						if (params.size() != 2)
						{
							return Hush ::Reflection ::Variant ::EVariantError ::NonSameType;
						}
						auto result = params[0].Get<Hush ::DirectionalLight>();
						if (result.has_error())
						{
							return result.error();
						}
						auto *instance = result.value();
						auto value = params[1].Get<Color>();
						if (value.has_error())
						{
							return value.error();
						}
						instance->color = *value.value();
						return {};
					},
					[](std ::span<const Hush ::Reflection ::VariantView> params)
						-> Hush ::Result<Hush ::Reflection ::Variant, Hush ::Reflection ::Variant ::EVariantError> {
						if (params.size() != 1)
						{
							return Hush ::Reflection ::Variant ::EVariantError ::NonSameType;
						}
						auto result = params[0].Get<Hush ::DirectionalLight>();
						if (result.has_error())
						{
							return result.error();
						}
						auto *instance = result.value();
						return Hush ::Reflection ::Variant(instance->color);
					},
					((::size_t) & reinterpret_cast<char const volatile &>((((Hush ::DirectionalLight *)0)->color)))))
				.Register();
		}
		template <typename T>
		Hush ::Serialization ::ESerializationError Serialize(T &serializer) const
		{
			auto error = serializer.template Serialize<std ::string_view>("__type", "Hush::DirectionalLight");
			if (error != Hush ::Serialization ::ESerializationError ::None)
			{
				return error;
			}
			if (auto result = serializer.Serialize("intensity", intensity);
				result != Hush ::Serialization ::ESerializationError ::None)
			{
				return result;
			}
			if (auto result = serializer.Serialize("color", color);
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
				Hush ::Serialization ::Visitor<float> intensityVisitor;
				Hush ::Serialization ::Visitor<Color> colorVisitor;
				enum class EVisitorStatus
				{
					None,
					INTENSITY,
					COLOR,
				};
				EVisitorStatus status = EVisitorStatus ::None;
				bool insideObject{false};
				explicit Visitor(IVisitor *parent, Hush ::DirectionalLight *instance,
								 Hush ::Serialization ::EFormatDescribingType format)
					: IVisitor(parent, format),
					  intensityVisitor(this, &instance->intensity, format),
					  colorVisitor(this, &instance->color, format)
				{
					(void)instance;
					if (format == Hush ::Serialization ::EFormatDescribingType ::NonSelfDescribing)
					{
						SetStartingVisitor(&intensityVisitor);
						colorVisitor.SetParentVisitor(GetParentVisitor());
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
					if (value == "intensity")
					{
						status = EVisitorStatus ::INTENSITY;
						return &intensityVisitor;
					}
					if (value == "color")
					{
						status = EVisitorStatus ::COLOR;
						return &colorVisitor;
					}
					return this;
				}
			};
			return Visitor{parent, this, format};
		}

	private:
	public:
		[[hush::property]]
		float intensity = 1.0F;
		[[hush::property]]
		Color color = Vector4Math::ONE;
	};

	void Serialize(DirectionalLight *component);
} // namespace Hush
