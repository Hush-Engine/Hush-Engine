#pragma once

#include "Transform.hpp"
#include "reflection/Annotations.hpp"

#if __has_include("WorldTransform.hushgen.hpp") && !defined(HUSH_HEADER_PARSING)
#include "WorldTransform.hushgen.hpp"
#endif

namespace Hush
{
	struct [[hush::export, hush::reflect(Hush::Reflection::name("WorldTransform"))]] WorldTransform : public Transform
	{
		HUSH_GENERATED_BODY
	};
} // namespace Hush
