#include "RenderPassNode.hpp"
#include "Assertions.hpp"

#include <algorithm>

Hush::RenderGraph::RenderPassNode::RenderPassNode(std::string_view name, uint32_t nodeId,
												  std::unique_ptr<PassBase> &&pass)
	: m_name(name),
	  m_pass(std::move(pass)),
	  m_nodeId(nodeId)
{
	constexpr static size_t initialReserve = 8;
	m_createsResources.reserve(initialReserve);
	m_readResources.reserve(initialReserve);
	m_writtenResources.reserve(initialReserve);
}

bool Hush::RenderGraph::RenderPassNode::ReadsResource(ResourceId id) const
{
	return std::ranges::find_if(m_readResources, [id](const PassAccess &access) { return access.resourceId == id; }) !=
		   m_readResources.end();
}

bool Hush::RenderGraph::RenderPassNode::WritesResource(ResourceId id) const
{
	return std::ranges::find_if(m_writtenResources, [id](const ResourceId &resId) { return resId == id; }) !=
		   m_writtenResources.end();
}

bool Hush::RenderGraph::RenderPassNode::CreatesResource(ResourceId id) const
{
	return std::ranges::find(m_createsResources, id) != m_createsResources.end();
}

Hush::RenderGraph::ResourceId Hush::RenderGraph::RenderPassNode::AddReadResource(ResourceId id)
{
	// Users MUST never issue a read for resources they create or write
	HUSH_ASSERT(!CreatesResource(id), "Cannot read a resource that is created by the same pass!");
	HUSH_ASSERT(!WritesResource(id), "Cannot read a resource that is written by the same pass!");

	const bool contains = ReadsResource(id);
	return contains ? id : m_readResources.emplace_back(PassAccess{id}).resourceId;
}

void Hush::RenderGraph::RenderPassNode::AddCreatedResource(ResourceId id)
{
	m_createsResources.emplace_back(id);
}

Hush::RenderGraph::ResourceId Hush::RenderGraph::RenderPassNode::AddWrittenResource(ResourceId id)
{
	return id;
}
