#include "Mesh.hpp"
#include <cstdint>

constexpr int32_t VERTEX_PER_TRIANGLE = 3;


void Hush::Mesh::CalculateNormals(Vertex& currentVertex) {
	(void)currentVertex;
	// NYI: We assume the model already has normals for now
}

void Hush::Mesh::CalculateTangentBasis() {
	for(int32_t i = 0; i < this->m_vertices.size(); i += VERTEX_PER_TRIANGLE) {
		glm::vec3& vertex0 = this->m_vertices.at(i).position;
		glm::vec3& vertex1 = this->m_vertices.at(i + 1).position;
		glm::vec3& vertex2 = this->m_vertices.at(i + 2).position;
		
		glm::vec2& uv0 = this->m_vertices.at(i).uv;
		glm::vec2& uv1 = this->m_vertices.at(i + 1).uv;
		glm::vec2& uv2 = this->m_vertices.at(i + 2).uv;

		glm::vec3 deltaPos1 = vertex1 - vertex0;
		glm::vec3 deltaPos2 = vertex2 - vertex0;

		glm::vec2 deltaUv1 = uv1 - uv0;
		glm::vec2 deltaUv2 = uv2 - uv0;
		
		float r = 1.0F / (deltaUv1.x * deltaUv2.y - deltaUv1.y * deltaUv2.x);
		
		glm::vec3 tangent = (deltaPos1 * deltaUv2.y   - deltaPos2 * deltaUv1.y) * r;
		// Bitangent will be calculated in the GPU, but do come back here if that is a bottleneck
		// glm::vec3 bitangent = (deltaPos2 * deltaUv1.x   - deltaPos1 * deltaUv2.x) * r;

		
		// Set the same tangent for all three vertices of the triangle.
        // They will be merged later, in vboindexer.cpp
        this->m_vertices.at(i + 0).tangent = tangent;
        this->m_vertices.at(i + 1).tangent = tangent;
        this->m_vertices.at(i + 2).tangent = tangent;
		// TODO: add handedness code
		
    }
}

