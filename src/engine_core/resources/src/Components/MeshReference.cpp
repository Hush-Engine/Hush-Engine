#include "MeshReference.hpp"

void Hush::MeshReference::CalculateBounds(glm::vec3 *outCenter, glm::vec3 *outSize) 
{
	HUSH_ASSERT(outCenter != nullptr && outSize != nullptr, "Center and size pointers should not be null!");
	const std::vector<Mesh::Vertex> &vertices = this->m_mesh->GetVertexBuffer();
	*outCenter = Vector3Math::ZERO;
	*outSize = Vector3Math::ONE;
	if (vertices.empty())
	{
		return;
	}
	auto min = glm::vec3(std::numeric_limits<float>::max());
	auto max = glm::vec3(std::numeric_limits<float>::min());

	for (const Mesh::Vertex &vertex : vertices)
	{
		// Adjust this line to match your Vertex's position field name.
		const glm::vec3 &pos = vertex.position;

		if (pos.x < min.x)
		{
			min.x = pos.x;
		}
		if (pos.y < min.y)
		{
			min.y = pos.y;
		}
		if (pos.z < min.z)
		{
			min.z = pos.z;
		}

		if (pos.x > max.x)
		{
			max.x = pos.x;
		}
		if (pos.y > max.y)
		{
			max.y = pos.y;
		}
		if (pos.z > max.z)
		{
			max.z = pos.z;
		}
	}
	*outCenter = (min + max) * 0.5f;
	*outSize = (max - min);
}
