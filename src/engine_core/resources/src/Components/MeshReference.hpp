#pragma once

#include "Components/Material3D.hpp"
#include "RHI/IGraphicsBuffer.hpp"
#include "Shared/Mesh.hpp"
#include "Ref.hpp"

#include <memory>
#include <vector>

namespace Hush
{
	/// @brief Small wrapper around the `Mesh` data structure, internally, this is useful for caching using our
	/// reference counting system
	// This structure is intended as a bridge between cached resource data and the rendering pipeline
	class MeshReference
	{
	public:
		MeshReference(Ref<Mesh> &mesh)
			: m_mesh(mesh)
		{
		}

		Ref<Mesh> &GetMesh()
		{
			return this->m_mesh;
		}

		[[nodiscard]]
		const Ref<Mesh> &GetMesh() const
		{
			return this->m_mesh;
		}

		/// @brief Get the GPU vertex buffer for this mesh (may be nullptr before upload).
		[[nodiscard]]
		Graphics::IGraphicsBuffer *GetGpuVertexBuffer() const
		{
			return m_gpuVertexBuffer.get();
		}

		/// @brief Set the GPU vertex buffer. Called by ResourceUploadSystem after staging.
		void SetGpuVertexBuffer(std::unique_ptr<Graphics::IGraphicsBuffer> buffer)
		{
			m_gpuVertexBuffer = std::move(buffer);
		}

		/// @brief Get the GPU index buffer for this mesh (may be nullptr before upload).
		[[nodiscard]]
		Graphics::IGraphicsBuffer *GetGpuIndexBuffer() const
		{
			return m_gpuIndexBuffer.get();
		}

		/// @brief Set the GPU index buffer. Called by ResourceUploadSystem after staging.
		void SetGpuIndexBuffer(std::unique_ptr<Graphics::IGraphicsBuffer> buffer)
		{
			m_gpuIndexBuffer = std::move(buffer);
		}

		void PushMaterial(Ref<Graphics::Material3D> &material)
		{
			this->m_materials.push_back(material);
		}

	private:
		Ref<Mesh> m_mesh;
		/// @brief Material references used for this mesh's GeometrySurfaces, see @ref GeoSurface
		std::vector<Ref<Graphics::Material3D>> m_materials;

		/// @brief GPU vertex buffer, owned by this component and populated by ResourceUploadSystem.
		std::unique_ptr<Graphics::IGraphicsBuffer> m_gpuVertexBuffer;

		/// @brief GPU index buffer, owned by this component and populated by ResourceUploadSystem.
		std::unique_ptr<Graphics::IGraphicsBuffer> m_gpuIndexBuffer;
	};

	void Serialize(MeshReference *component, const char *entityName);
} // namespace Hush
