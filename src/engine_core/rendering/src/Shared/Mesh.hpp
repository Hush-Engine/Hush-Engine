#pragma once

#include "Shared/IMaterial3D.hpp"
#include "Vector3Math.hpp"
#include "Vulkan/GPUMeshBuffers.hpp"
#include <glm/ext/vector_float2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <string_view>
#include <vector>

namespace Hush
{
	struct GeoSurface
	{
		uint32_t startIndex;
		uint32_t count;
		std::shared_ptr<IMaterial3D> material;
	};

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

		[[nodiscard]]
		const std::vector<GeoSurface> &GetSurfaces() const
		{
			return this->m_surfaces;
		}

		void AddSurface(GeoSurface &&surface)
		{
			this->m_surfaces.emplace_back(surface);
		}

		void SetName(const std::string_view &name)
		{
			this->m_name = name;
		}

		[[nodiscard]]
		const std::string &GetName() const
		{
			return this->m_name;
		}

		void SetMeshBuffers(GPUMeshBuffers buffers)
		{
			this->m_meshBuffers = buffers;
		}

		GPUMeshBuffers &GetMeshBuffers()
		{
			return this->m_meshBuffers;
		}

		[[nodiscard]]
		const GPUMeshBuffers &GetMeshBuffers() const
		{
			return this->m_meshBuffers;
		}

	private:
		void CalculateNormals(Vertex &currentVertex);
		std::vector<uint32_t> m_indices;
		std::vector<Vertex> m_vertices;
		std::string m_name;
		std::vector<GeoSurface> m_surfaces;
		GPUMeshBuffers m_meshBuffers;
	};

	void Serialize(Mesh *component);
} // namespace Hush
