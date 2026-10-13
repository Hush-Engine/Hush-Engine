#pragma once

#include "Transform.hpp"
#include "reflection/Annotations.hpp"

#if __has_include("LocalTransform.hushgen.hpp") && !defined(HUSH_HEADER_PARSING)
#include "LocalTransform.hushgen.hpp"
#endif

namespace Hush
{
	struct [[hush::export, hush::reflect(Hush::Reflection::name("LocalTransform"))]] LocalTransform : public Transform
	{
		HUSH_GENERATED_BODY
	};
} // namespace Hush
