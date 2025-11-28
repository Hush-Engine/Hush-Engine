/*! \file RenderGraph.cpp
	\author Alan Ramirez Herrera
	\date 2025-11-17
	\brief RenderGraph implementation for rendering
*/

#include "RenderGraph.hpp"
#include "RenderGraph/ResourceId.hpp"
#include "RenderGraph/ResourceNode.hpp"
#include "Assertions.hpp"

#include <stack>

void Hush::RenderGraph::RenderGraph::Compile()
{
    // Based on the frostbite presentation, the algorithm to compile the render graph is as follows:
    // Compute initial resource and pass reference counts.
    //      renderPass.refCount++ for each resource written by the pass
    //      resource.refCount++ for each resource read
    // Identify resource with zero ref count and push them on a stack.
    // While the stack is not empty:
    //      Pop a resource from the stack and decrement the ref count of its producer pass.
    //      If producer.refCount == 0, decrement the ref count of all resources read by the pass.
    //          Add them to the stack when their ref count reaches zero.
	for (auto &pass : m_passes)
	{
	    // pass.refcount = number of written resources
		pass.m_refCount = static_cast<int32_t>(pass.m_writtenResources.size());

		// Now, for each resource read by the pass, increment its refcount
		for (const auto& resource : pass.m_readResources)
		{
		    auto& resourceNode = m_resources[resource.resourceId];
            resourceNode.m_refCount++;
		}
		// Same for written resources
		for (const auto& resourceId : pass.m_writtenResources)
        {
            auto& resourceNode = m_resources[resourceId];
            resourceNode.m_refCount++;
        }
	}

	// Now we need to proceed to cull passes that do not contribute to final outputs.

	std::stack<ResourceNode*> zeroRefResources;
	for (ResourceNode& resourceNode : m_resources)
	{
	    if (resourceNode.m_refCount == 0)
        {
            zeroRefResources.push(&resourceNode);
        }
	}

	while (!zeroRefResources.empty())
    {
        ResourceNode* zeroRefResourceNode = zeroRefResources.top();
        zeroRefResources.pop();

        RenderPassNode* producerPass = zeroRefResourceNode->m_producer;
        if (producerPass == nullptr || producerPass->GetCullingMode() == RenderPassNode::EPassCullingMode::NeverCull)
        {
            continue;
        }

        HUSH_ASSERT(producerPass->m_refCount >= 1, "Producer pass ref count should be at least 1");
        producerPass->m_refCount--;

        if (producerPass->m_refCount == 0)
        {
            // This pass can be culled, so we need to decrement the ref count of all resources read by the pass
            for (const auto& readResource : producerPass->m_readResources)
            {
                ResourceNode& readResourceNode = m_resources[readResource.resourceId];
                HUSH_ASSERT(readResourceNode.m_refCount >= 1, "Read resource ref count should be at least 1");
                readResourceNode.m_refCount--;

                if (readResourceNode.m_refCount == 0)
                {
                    zeroRefResources.push(&readResourceNode);
                }
            }
        }
    }

	// Now, we can calculate the lifetime of each resource
	for (RenderPassNode& pass : m_passes)
	{
	    if (pass.m_refCount == 0) continue;

		for (const ResourceId id : pass.m_createsResources)
		{

		}
	}

}



void Hush::RenderGraph::RenderGraph::Execute(void *ctx, void *allocator)
{
}
