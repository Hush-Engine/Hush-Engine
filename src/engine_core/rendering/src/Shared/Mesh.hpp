#pragma once

#include "Vector3Math.hpp"
#include "Shared/GPUMeshBuffers.hpp"
#include "RHI/IGraphicsBuffer.hpp"
#include <glm/ext/vector_float2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace Hush
{

	namespace Graphics
	{
		class Material3D;
	}

	struct GeoSurface
	{
		uint32_t startIndex;
		uint32_t count;
		// Lifetime of this material is handled by the MeshReference component, this is the identifier of the
		// Ref<Material3D>
		uint32_t materialResource;
	};

	/// @brief Simple CPU representation of a mesh "component", holds index and vertex buffers, as well as the
	/// RenderingAPI specific buffer data
	class Mesh
	{
	public:
#pragma warning(push)
#pragma warning(disable : 4324)
		// NOTE (Nef): I removed alignas properties because it caused a mismatch with Slang's layout (we were expecting 64 bytes of stride and we ended up with 80)
		struct Vertex
		{
			glm::vec3 position{};
			glm::vec3 normal = Vector3Math::RIGHT;
			glm::vec4 color = glm::vec4{1.f};
			glm::vec4 tangent;
			glm::vec2 uv{};
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

		[[nodiscard]]
		std::vector<GeoSurface> &GetSurfaces()
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

		void CalculateNormals();

		/// @brief Get the shared GPU vertex buffer (nullptr before upload).
		[[nodiscard]]
		Graphics::IGraphicsBuffer *GetGpuVertexBuffer() const
		{
			return m_gpuVertexBuffer.get();
		}

		/// @brief Get the shared GPU index buffer (nullptr before upload).
		[[nodiscard]]
		Graphics::IGraphicsBuffer *GetGpuIndexBuffer() const
		{
			return m_gpuIndexBuffer.get();
		}

		/// @brief Set the shared GPU vertex buffer. Called by ResourceUploadSystem after staging.
		void SetGpuVertexBuffer(std::unique_ptr<Graphics::IGraphicsBuffer> buffer)
		{
			m_gpuVertexBuffer = std::move(buffer);
		}

		/// @brief Set the shared GPU index buffer. Called by ResourceUploadSystem after staging.
		void SetGpuIndexBuffer(std::unique_ptr<Graphics::IGraphicsBuffer> buffer)
		{
			m_gpuIndexBuffer = std::move(buffer);
		}

	private:
		std::vector<uint32_t> m_indices;
		std::vector<Vertex> m_vertices;
		std::string m_name;
		std::vector<GeoSurface> m_surfaces;
		GPUMeshBuffers m_meshBuffers;

		/// @brief Shared GPU vertex buffer,owned here so all MeshReference instances
		/// pointing to this Mesh reuse the same buffer instead of duplicating it.
		std::unique_ptr<Graphics::IGraphicsBuffer> m_gpuVertexBuffer;
		std::unique_ptr<Graphics::IGraphicsBuffer> m_gpuIndexBuffer;
	};
} // namespace Hush
