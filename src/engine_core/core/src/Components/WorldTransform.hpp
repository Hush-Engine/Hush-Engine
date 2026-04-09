#pragma once

#include "Transform.hpp"
namespace Hush
{
	struct [[hush::export]] WorldTransform : public Transform
	{
		using Transform::Transform;
	};
} // namespace Hush
