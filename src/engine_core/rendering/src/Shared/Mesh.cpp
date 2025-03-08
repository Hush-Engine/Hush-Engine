#include "Mesh.hpp"
#include "Vector4Math.hpp"

constexpr size_t VERTEX_PER_TRIANGLE = 3;


void Hush::Mesh::CalculateNormals(Vertex& currentVertex) {
	(void)currentVertex;
	// NYI: We assume the model already has normals for now
}

void Hush::Mesh::CalculateTangentBasis() {

	for(size_t i = 0; i < this->m_indices.size(); i += VERTEX_PER_TRIANGLE) {

		// Get the actual indices for the polygon we're working with
		size_t idx0 = this->m_indices.at(i);
		size_t idx1 = this->m_indices.at(i + 1);
		size_t idx2 = this->m_indices.at(i + 2);
		
		glm::vec3& vertex0 = this->m_vertices.at(idx0).position;
		glm::vec3& vertex1 = this->m_vertices.at(idx1).position;
		glm::vec3& vertex2 = this->m_vertices.at(idx2).position;
		
		glm::vec2& uv0 = this->m_vertices.at(idx0).uv;
		glm::vec2& uv1 = this->m_vertices.at(idx1).uv;
		glm::vec2& uv2 = this->m_vertices.at(idx2).uv;

		glm::vec3 deltaPos1 = vertex1 - vertex0;
		glm::vec3 deltaPos2 = vertex2 - vertex0;

		glm::vec2 deltaUv1 = uv1 - uv0;
		glm::vec2 deltaUv2 = uv2 - uv0;
		
		float r = 1.0F / (deltaUv1.x * deltaUv2.y - deltaUv1.y * deltaUv2.x);
		
		float handedness = ((deltaPos1.y * deltaPos2.x - deltaPos2.y * deltaPos1.x) < 0.0F) ? -1.0F : 0.0F;
		glm::vec3 tangent = (deltaPos1 * deltaUv2.y   - deltaPos2 * deltaUv1.y) * r;
		// Bitangent will be calculated in the GPU
		
		// Set the same tangent for all three vertices of the triangle.
        // They will be merged later, in vboindexer.cpp

		glm::vec4 tangWithHandedness = Vector4Math::FromVec3(tangent, handedness);
        this->m_vertices.at(idx0).tangent = tangWithHandedness;
        this->m_vertices.at(idx1).tangent = tangWithHandedness;
        this->m_vertices.at(idx2).tangent = tangWithHandedness;
    }
    
}

