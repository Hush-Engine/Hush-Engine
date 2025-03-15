#pragma once

#include "Assertions.hpp"
#include "Vector3Math.hpp"
#include <glm/ext/vector_float2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <vector>

namespace Hush
{
	/// @brief Simple CPU representation of a mesh "component", holds index and vertex buffers, as well as the
	/// RenderingAPI specific buffer data
	class Mesh
	{
	public:
#pragma warning(push)
#pragma warning(disable : 4324)
		struct Vertex
		{
			alignas(16) glm::vec3 position{};
			alignas(16) glm::vec3 normal = Vector3Math::RIGHT;
			alignas(16) glm::vec4 color = glm::vec4{1.f};
			alignas(16) glm::vec4 tangent;
			alignas(8) glm::vec2 uv{};
		};

#pragma warning(pop)

		// HUSH_STATIC_ASSERT(sizeof(Vertex) % 16 == 0);

		[[nodiscard]]
		inline std::vector<uint32_t> &GetIndexBuffer()
		{
			return this->m_indices;
		}

		[[nodiscard]]
		inline std::vector<Vertex> &GetVertexBuffer()
		{
			return this->m_vertices;
		}

		void CalculateTangentBasis();

	private:
		void CalculateNormals(Vertex &currentVertex);
		std::vector<uint32_t> m_indices;
		std::vector<Vertex> m_vertices;
	};
} // namespace Hush
