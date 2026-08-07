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
		HUSH_GENERATED_BODY
	public:
		[[hush::property]]
		float intensity = 1.0F;
		// [[hush::property]]
		Color color = Vector4Math::ONE;
	};

	void Serialize(DirectionalLight *component);
} // namespace Hush
