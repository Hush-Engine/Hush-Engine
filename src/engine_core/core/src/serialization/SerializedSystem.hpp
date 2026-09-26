#pragma once

#include <Hushgen.hpp>

#include <reflection/Type.hpp>
#include <serialization/Deserialization.hpp>
#include <serialization/Serialization.hpp>
#include <string>
#include <type_traits>

#if __has_include("SerializedSystem.hushgen.hpp") && !defined(HUSH_HEADER_PARSING)
#include "SerializedSystem.hushgen.hpp"
#endif

namespace Hush
{
	/// Stable identity of a module-owned system stored in a scene asset.
	struct [[hush::reflect]] SerializedSystem
	{
		HUSH_GENERATED_BODY
	public:
		[[hush::property]]
		std::string module;

		[[hush::property]]
		std::string type;
	};
} // namespace Hush
