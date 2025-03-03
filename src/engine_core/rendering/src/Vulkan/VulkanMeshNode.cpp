#include "VulkanLoader.hpp"
#include "VulkanMeshNode.hpp"
#include "VkRenderObject.hpp"
#include "Assertions.hpp"
#include "DrawContext.hpp"

Hush::VulkanMeshNode::VulkanMeshNode(std::shared_ptr<MeshAsset> mesh)
{
	this->m_mesh = mesh;
}

void Hush::VulkanMeshNode::Draw(const glm::mat4& topMatrix, void* drawContext)
{
	//Interpret drawContext as: std::vector<VkRenderObject>* OpaqueSurfaces;
	HUSH_ASSERT(drawContext != nullptr, "Draw context should not be null for any render node");
	auto* drawCtxImpl = static_cast<DrawContext*>(drawContext);
	glm::mat4 nodeMatrix = topMatrix * this->m_worldTransform;

	for (GeoSurface& s : this->m_mesh->surfaces) {
		VkRenderObject def{};
		def.indexCount = s.count;
		def.firstIndex = s.startIndex;
		def.indexBuffer = this->m_mesh->meshBuffers.indexBuffer.GetBuffer();
		def.material = s.material.get();

		def.transform = nodeMatrix;
		def.vertexBufferAddress = this->m_mesh->meshBuffers.vertexBufferAddress;
		if (s.material->passType == EMaterialPass::Transparent) {
			drawCtxImpl->transparentSurfaces.push_back(def);
		}
		else {
			drawCtxImpl->opaqueSurfaces.push_back(def);
		}
	}

	RenderableNode::Draw(topMatrix, drawContext);
}

Hush::MeshAsset& Hush::VulkanMeshNode::GetMesh()
{
	return *this->m_mesh.get();
}

void Hush::VulkanMeshNode::SetMaterialDataBuffer(VulkanAllocatedBuffer materialDataBuffer)
{
	this->m_materialDataBuffer = materialDataBuffer;
}

void Hush::VulkanMeshNode::SetDescriptorPool(DescriptorAllocatorGrowable descriptorPool)
{
	this->m_descriptorPool = descriptorPool;
}

const Hush::VulkanAllocatedBuffer& Hush::VulkanMeshNode::GetMaterialDataBuffer() const noexcept
{
	return this->m_materialDataBuffer;
}

const Hush::DescriptorAllocatorGrowable& Hush::VulkanMeshNode::GetDescriptorPool() const noexcept
{
	return this->m_descriptorPool;
}
