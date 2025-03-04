#pragma once

#include <glm/mat4x4.hpp>
#include <vector>

namespace Hush
{
	class IRenderable
	{
	public:
		IRenderable() = default;

		virtual ~IRenderable() = default;

		virtual void Draw(const glm::mat4 &topMatrix, void *drawContext) = 0;
	};
} // namespace Hush