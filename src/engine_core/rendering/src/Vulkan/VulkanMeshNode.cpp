#include "Shared/GpuAllocatedBuffer.hpp"
#include "Shared/Mesh.hpp"
#include "VulkanLoader.hpp"
#include "VulkanMeshNode.hpp"

#include <utility>
#include "VkRenderObject.hpp"
#include "Assertions.hpp"
#include "DrawContext.hpp"

Hush::VulkanMeshNode::VulkanMeshNode(std::shared_ptr<Mesh> mesh)
	: m_mesh(std::move(mesh))
{
}

void Hush::VulkanMeshNode::Draw(const glm::mat4 &topMatrix, void *drawContext)
{
	// Interpret drawContext as: std::vector<VkRenderObject>* OpaqueSurfaces;
	HUSH_ASSERT(drawContext != nullptr, "Draw context should not be null for any render node");
	auto *drawCtxImpl = static_cast<DrawContext *>(drawContext);
	glm::mat4 nodeMatrix = topMatrix * this->m_worldTransform;

	for (const GeoSurface &s : this->m_mesh->GetSurfaces())
	{
		VkRenderObject def{};
		def.indexCount = s.count;
		def.firstIndex = s.startIndex;
		def.indexBuffer = static_cast<VkBuffer>(this->m_mesh->GetMeshBuffers().indexBuffer.GetBuffer());
		// Replace with graphics API call
		def.material = s.material->GetInternalMaterial();

		def.transform = nodeMatrix;
		def.vertexBufferAddress = this->m_mesh->GetMeshBuffers().vertexBufferAddress;
		if (s.material->GetInternalMaterial()->passType == EMaterialPass::Transparent)
		{
			drawCtxImpl->transparentSurfaces.push_back(def);
		}
		else
		{
			drawCtxImpl->opaqueSurfaces.push_back(def);
		}
	}

	RenderableNode::Draw(topMatrix, drawContext);
}

Hush::Mesh &Hush::VulkanMeshNode::GetMesh()
{
	return *this->m_mesh;
}

void Hush::VulkanMeshNode::SetMaterialDataBuffer(GpuAllocatedBuffer materialDataBuffer)
{
	this->m_materialDataBuffer = materialDataBuffer;
}

void Hush::VulkanMeshNode::SetDescriptorPool(DescriptorAllocatorGrowable descriptorPool)
{
	this->m_descriptorPool = descriptorPool;
}

const Hush::GpuAllocatedBuffer &Hush::VulkanMeshNode::GetMaterialDataBuffer() const noexcept
{
	return this->m_materialDataBuffer;
}

const Hush::DescriptorAllocatorGrowable &Hush::VulkanMeshNode::GetDescriptorPool() const noexcept
{
	return this->m_descriptorPool;
}
