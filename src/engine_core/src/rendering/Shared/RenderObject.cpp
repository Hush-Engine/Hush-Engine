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

glm::mat4 &Hush::INode::GetLocalTransform() noexcept
{
    return this->m_localTransform;
}

std::weak_ptr<Hush::INode> Hush::INode::GetParent()
{
    return this->GetParent();
}

void Hush::INode::SetParent(std::weak_ptr<INode> parent)
{
    this->m_parent = parent;
}

void Hush::INode::SetTransform(const glm::mat4 &matrix) noexcept {
    this->m_localTransform = matrix;
}

void Hush::INode::AddChild(std::shared_ptr<INode> child)
{
    this->m_children.push_back(child);
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

void Hush::LoadedGLTF::AddTopNode(std::shared_ptr<INode> node)
{
    this->m_topNodes.push_back(node);
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

void Hush::MeshNode::Draw(const glm::mat4 &topMatrix, DrawContext &ctx)
{
    glm::mat4 nodeMatrix = topMatrix * this->m_worldTransform;

    for (auto &s : mesh->surfaces)
    {
        RenderObject def;
        def.indexCount = s.count;
        def.firstIndex = s.startIndex;
        def.indexBuffer = mesh->meshBuffers.indexBuffer.GetBuffer();
        def.material = &s.material->data;

        def.transform = nodeMatrix;
        def.vertexBufferAddress = mesh->meshBuffers.vertexBufferAddress;

        ctx.opaqueSurfaces.push_back(def);
    }

    // recurse down
    INode::Draw(topMatrix, ctx);
}
