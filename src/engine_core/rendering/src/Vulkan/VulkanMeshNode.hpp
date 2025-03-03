#pragma once
#include "Shared/RenderableNode.hpp"
#include "VkDescriptors.hpp"

namespace Hush
{
    struct MeshAsset;

    class VulkanMeshNode final : public RenderableNode
    {

    public:
        // TODO: remove from public stuff
        std::shared_ptr<MeshAsset> m_mesh;
        DescriptorAllocatorGrowable m_descriptorPool;

        VulkanMeshNode(std::shared_ptr<MeshAsset> mesh);

        void Draw(const glm::mat4 &topMatrix, void *drawContext) override;

        MeshAsset &GetMesh();

        void SetMaterialDataBuffer(VulkanAllocatedBuffer materialDataBuffer);
        void SetDescriptorPool(DescriptorAllocatorGrowable descriptorPool);

        const VulkanAllocatedBuffer &GetMaterialDataBuffer() const noexcept;
        const DescriptorAllocatorGrowable &GetDescriptorPool() const noexcept;

    private:
        VulkanAllocatedBuffer m_materialDataBuffer;
    };
} // namespace Hush
