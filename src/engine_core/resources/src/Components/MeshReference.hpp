#pragma once

#include "Shared/Mesh.hpp"
#include "Ref.hpp"

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
	private:
		Ref<Mesh> m_mesh;
	};

	void Serialize(MeshReference *component, const char *entityName);
} // namespace Hush
