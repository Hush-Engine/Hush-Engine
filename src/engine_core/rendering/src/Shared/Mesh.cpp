#include "Mesh.hpp"
#include "Vector4Math.hpp"
#include <glm/ext/vector_float3.hpp>
#include <glm/geometric.hpp>

constexpr size_t VERTEX_PER_TRIANGLE = 3;

void Hush::Mesh::CalculateNormals()
{
	for (size_t i = 0; i < this->m_indices.size(); i += VERTEX_PER_TRIANGLE)
	{

		size_t idx0 = this->m_indices.at(i);
		size_t idx1 = this->m_indices.at(i + 1);
		size_t idx2 = this->m_indices.at(i + 2);

		Vertex &vertex0 = this->m_vertices.at(idx0);
		Vertex &vertex1 = this->m_vertices.at(idx1);
		Vertex &vertex2 = this->m_vertices.at(idx2);
		// n = normalize(cross(b-a, c-a))
		glm::vec3 faceCross = glm::cross(vertex1.position - vertex0.position, vertex2.position - vertex0.position);
		glm::vec3 normal = glm::normalize(faceCross);
		vertex0.normal = normal;
		vertex1.normal = normal;
		vertex2.normal = normal;
	}
}

void Hush::Mesh::CalculateTangentBasis()
{
    // Initialize tangents to zero for all vertices first
    for (auto& vert : m_vertices) {
        vert.tangent = glm::vec4(0.0f);
    }

    std::vector<glm::vec3> tan1(m_vertices.size(), glm::vec3(0.0f));
    std::vector<glm::vec3> tan2(m_vertices.size(), glm::vec3(0.0f));

    for (size_t i = 0; i < this->m_indices.size(); i += VERTEX_PER_TRIANGLE)
    {
        size_t idx0 = this->m_indices.at(i);
        size_t idx1 = this->m_indices.at(i + 1);
        size_t idx2 = this->m_indices.at(i + 2);

        glm::vec3 &v0 = this->m_vertices.at(idx0).position;
        glm::vec3 &v1 = this->m_vertices.at(idx1).position;
        glm::vec3 &v2 = this->m_vertices.at(idx2).position;

        glm::vec2 &uv0 = this->m_vertices.at(idx0).uv;
        glm::vec2 &uv1 = this->m_vertices.at(idx1).uv;
        glm::vec2 &uv2 = this->m_vertices.at(idx2).uv;

        glm::vec3 deltaPos1 = v1 - v0;
        glm::vec3 deltaPos2 = v2 - v0;
        glm::vec2 deltaUv1 = uv1 - uv0;
        glm::vec2 deltaUv2 = uv2 - uv0;

        float det = deltaUv1.x * deltaUv2.y - deltaUv1.y * deltaUv2.x;
        
        // Guard against degenerate UV triangles
        if (std::abs(det) < 1e-8f) {
            continue; // Skip degenerate triangles
        }

        float r = 1.0f / det;

        // Correct tangent calculation
        glm::vec3 tangent = (deltaPos1 * deltaUv2.y - deltaPos2 * deltaUv1.y) * r;
        glm::vec3 bitangent = (deltaPos2 * deltaUv1.x - deltaPos1 * deltaUv2.x) * r;

        // Accumulate for smooth averaging
        tan1[idx0] += tangent;
        tan1[idx1] += tangent;
        tan1[idx2] += tangent;
        tan2[idx0] += bitangent;
        tan2[idx1] += bitangent;
        tan2[idx2] += bitangent;
    }

    // Finalize: Orthonormalize, normalize, and calculate the `w` handedness
    for (size_t i = 0; i < m_vertices.size(); ++i)
    {
        glm::vec3 n = glm::normalize(m_vertices[i].normal);
        glm::vec3 t = tan1[i];

        // Gram-Schmidt orthonormalization
        t = glm::normalize(t - n * glm::dot(n, t));

        glm::vec3 bitangent = tan2[i];
        // Correct handedness using the cross product of the accumulated vectors
        float handedness = (glm::dot(glm::cross(n, t), bitangent) < 0.0f) ? -1.0f : 1.0f;

        m_vertices[i].tangent = glm::vec4(t, handedness);
    }
}
