#pragma once

#include <glm/mat4x4.hpp>
#include <memory>
#include <utility>
#include "IRenderable.hpp"
#include "Shared/Mesh.hpp"
namespace Hush
{
	/// @brief Common renderable node for scenes with multiple children to render
	/// the Draw function *MUST* be called (recursed down) for every implementation
	/// PENDING: Unite this into one renderableNode implementation IF AND ONLY IF this becomes a bottleneck
	class RenderableNode : public IRenderable
	{

	public:

		RenderableNode() = default;
		
		RenderableNode(std::shared_ptr<Mesh> mesh) : m_mesh(std::move(mesh)) {
			
		}

		void Draw(const glm::mat4 &topMatrix, void *drawContext) override
		{
			(void)topMatrix;
			(void)drawContext;
		}
		
		Mesh &GetMesh() {
			return *this->m_mesh;
		}
		
	protected:
		// NOLINTNEXTLINE
		glm::mat4 m_worldTransform{};
	private:
		std::shared_ptr<Mesh> m_mesh = nullptr;
	};
} // namespace Hush
