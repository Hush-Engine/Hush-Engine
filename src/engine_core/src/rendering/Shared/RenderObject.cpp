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

DescriptorAllocatorGrowable& Hush::LoadedGLTF::GetDescriptorPool() noexcept {
	return this->m_descriptorPool;
}
