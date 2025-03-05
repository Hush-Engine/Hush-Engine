#pragma once

#include "Vector3Math.hpp"
#include <glm/ext/vector_float2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <vector>

namespace Hush {
	/// @brief Simple CPU representation of a mesh "component", holds index and vertex buffers, as well as the RenderingAPI specific buffer data
	class Mesh {
	public:
		struct Vertex
		{
			glm::vec3 position{};
			glm::vec2 uv{};
			glm::vec3 normal = Vector3Math::RIGHT;
			glm::vec4 color = glm::vec4{1.f};
			glm::vec3 tangent;
		};
		
		[[nodiscard]] inline std::vector<uint32_t>& GetIndexBuffer() {
			return this->m_indices;
		}

		[[nodiscard]] inline std::vector<Vertex>& GetVertexBuffer() {
			return this->m_vertices;
		}
		
	private:
		void CalculateNormals(Vertex& currentVertex);
		
		void CalculateTangentBasis();
		
		std::vector<uint32_t> m_indices;
		std::vector<Vertex> m_vertices;
	};
}


