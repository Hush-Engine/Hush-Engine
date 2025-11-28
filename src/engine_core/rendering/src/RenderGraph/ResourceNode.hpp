/*! \file ResourceNode.hpp
	\author Alan Ramirez Herrera
	\date 2025-11-17
	\brief Represents a resource inside of the render graph.
*/

#pragma once
#include <cstdint>
#include <string>
#include <string_view>

namespace Hush::RenderGraph
{
    class RenderPassNode;

    /// Represents a resource inside of the render graph
	class ResourceNode final
	{
		friend class RenderGraph;

	public:
		ResourceNode(const ResourceNode &) = delete;
		ResourceNode(ResourceNode &&) = default;

		ResourceNode &operator=(const ResourceNode &) = delete;
		ResourceNode &operator=(ResourceNode &&) = delete;

		~ResourceNode() = default;

		[[nodiscard]]
		uint32_t GetNodeId() const noexcept
		{
			return m_nodeId;
		}

		[[nodiscard]]
		uint32_t GetResourceId() const noexcept
		{
			return m_resourceId;
		}

		[[nodiscard]]
		uint32_t GetVersion() const noexcept
		{
			return m_version;
		}

	private:
		ResourceNode(std::string_view name, uint32_t nodeId, uint32_t resourceId, uint32_t version)
			: m_name(name),
			  m_nodeId(nodeId),
			  m_resourceId(resourceId),
			  m_version(version)
		{
        }

	private:
		std::string m_name;
		RenderPassNode *m_producer = nullptr;
		RenderPassNode *m_lastConsumer = nullptr;
		const uint32_t m_nodeId;
		int32_t m_refCount = 0;
		const uint32_t m_resourceId;
		const uint32_t m_version;
	};
}
