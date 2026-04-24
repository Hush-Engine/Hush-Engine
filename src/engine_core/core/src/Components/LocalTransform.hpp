#pragma once

#include "Transform.hpp"

namespace Hush
{
	struct [[hush::export]] LocalTransform : public Transform
	{
		using Transform::Transform;
	};
} // namespace Hush
