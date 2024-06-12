/*! \file GltfMetallicRoughness.hpp
    \author Kyn21kx
    \date 2024-05-31
    \brief Describes a GLTF asset to be loaded
*/

#pragma once
#include <rendering/Shared/MaterialDefinitions.hpp>
#include "VkTypes.hpp"
#include "VkDescriptors.hpp"
#include <rendering/Renderer.hpp>
namespace Hush
{
    struct GLTFMetallicRoughness
    {
        MaterialPipeline opaquePipeline;
        MaterialPipeline transparentPipeline;

        VkDescriptorSetLayout materialLayout;

        struct MaterialConstants
        {
            glm::vec4 colorFactors;
            glm::vec4 metalRoughFactors;
            // padding, we need it anyway for uniform buffers
            glm::vec4 extra[14];
        };

        struct MaterialResources
        {
            AllocatedImage colorImage;
            VkSampler colorSampler;
            AllocatedImage metalRoughImage;
            VkSampler metalRoughSampler;
            VkBuffer dataBuffer;
            uint32_t dataBufferOffset;
        };

        DescriptorWriter writer;

        void BuildPipelines(IRenderer *engine);
        void ClearResources(VkDevice device);

        MaterialInstance WriteMaterial(VkDevice device, EMaterialPass pass, const MaterialResources &resources,
                                        DescriptorAllocatorGrowable &descriptorAllocator);
    };
}