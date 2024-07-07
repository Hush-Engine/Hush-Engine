#include "RenderObject.hpp"
#include "rendering/Renderer.hpp"
#include "rendering/Vulkan/VkDescriptors.hpp"

void Hush::LoadedGLTF::ClearAll()
{

}

void Hush::INode::RefreshTransform(const glm::mat4 &parentMatrix) {
	this->m_worldTransform = parentMatrix * m_localTransform;
	for (std::shared_ptr<INode>& c : m_children) {
		//Step into every child and update theirs too
    	c->RefreshTransform(m_worldTransform);
	}
}

void Hush::INode::Draw(const glm::mat4 &topMatrix, DrawContext &ctx) {
	// Draw children
	for (std::shared_ptr<INode>& c : m_children) {
	    c->Draw(topMatrix, ctx);
	}
}

Hush::IRenderer* Hush::LoadedGLTF::GetCreator() const noexcept {
	return this->m_creator;
}

void Hush::LoadedGLTF::SetCreator(IRenderer *creator) {
	this->m_creator = creator;
}

void Hush::LoadedGLTF::AddSampler(VkSampler sampler)
{
    this->m_samplers.push_back(sampler);
}

void Hush::LoadedGLTF::AddImage(const std::string_view &name, AllocatedImage image)
{
    this->m_images.insert_or_assign(name, image);
}

void Hush::LoadedGLTF::SetMaterialDataBuffer(VulkanVertexBuffer &materialDataBuffer)
{
    this->m_materialDataBuffer = materialDataBuffer;
}

void Hush::LoadedGLTF::AddMaterial(const std::string_view &name, std::shared_ptr<GLTFMaterial> materialPtr)
{
	//FIXME: Throws
    this->m_materials.insert_or_assign(name, materialPtr);
}

void Hush::LoadedGLTF::AddMesh(const std::string_view &name, std::shared_ptr<MeshAsset> mesh)
{
    // FIXME: Throws
    this->m_meshes.insert_or_assign(name, mesh);
}

void Hush::LoadedGLTF::AddNode(const std::string_view &name, std::shared_ptr<INode> node)
{
    // FIXME: Throws
    this->m_nodes.insert_or_assign(name, node);
}

VkSampler &Hush::LoadedGLTF::GetSampler(uint32_t index)
{
    return this->m_samplers.at(index);
}

DescriptorAllocatorGrowable &Hush::LoadedGLTF::GetDescriptorPool() noexcept
{
	return this->m_descriptorPool;
}

std::shared_ptr<Hush::MeshAsset> Hush::LoadedGLTF::GetMesh(const std::string_view &&name) noexcept
{
    return this->m_meshes.at(name);
}

Hush::VulkanVertexBuffer& Hush::LoadedGLTF::GetMaterialDataBuffer() noexcept
{
    return this->m_materialDataBuffer;
}

std::shared_ptr<Hush::GLTFMaterial> Hush::LoadedGLTF::GetMaterialOwning(const std::string_view &name) noexcept
{
	//TODO: Add safety check to see if the map actually contains the key
    return this->m_materials.at(name);
}
