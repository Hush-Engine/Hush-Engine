/*! \file RenderObject.hpp
    \author Kyn21kx
    \date 2024-05-30
    \brief Basic struct of a renderable object
*/

#pragma once
#include <vulkan/vulkan.h>
#include "MaterialDefinitions.hpp"
#include <unordered_map>
#include <vector>
#include <array>
#include <string>
#include <rendering/Vulkan/VkTypes.hpp>
#include "../Vulkan/VulkanVertexBuffer.hpp"
#include <rendering/Renderer.hpp>

struct Bounds
{
    glm::vec3 origin;
    float sphereRadius;
    glm::vec3 extents;
};

struct RenderObject
{
    uint32_t indexCount;
    uint32_t firstIndex;
    VkBuffer indexBuffer; //TODO: Make this a generic index buffer class

    MaterialInstance *material;
    Bounds bounds;
    glm::mat4 transform;
    VkDeviceAddress vertexBufferAddress;

    [[nodiscard]] bool IsVisible(const glm::mat4 &viewProjection) const
    {
        std::array<glm::vec3, 8> corners{
            glm::vec3{1, 1, 1},  glm::vec3{1, 1, -1},  glm::vec3{1, -1, 1},  glm::vec3{1, -1, -1},
            glm::vec3{-1, 1, 1}, glm::vec3{-1, 1, -1}, glm::vec3{-1, -1, 1}, glm::vec3{-1, -1, -1},
        };

        glm::mat4 matrix = viewProjection * this->transform;

        glm::vec3 min = {1.5, 1.5, 1.5};
        glm::vec3 max = {-1.5, -1.5, -1.5};

        for (int c = 0; c < 8; c++)
        {
            // project each corner into clip space
            glm::vec4 v = matrix * glm::vec4(this->bounds.origin + (corners.at(c) * this->bounds.extents), 1.0f);

            // perspective correction
            v.x = v.x / v.w;
            v.y = v.y / v.w;
            v.z = v.z / v.w;
            glm::vec3 viewVector = {v.x, v.y, v.z};
            min = glm::min<glm::vec3>(viewVector, min);
            max = glm::max<glm::vec3>(viewVector, max);
        }

        // check the clip space box is within the view
        return (min.z <= 1.f && max.z >= 0.f && min.x <= 1.f && max.x >= -1.f && min.y <= 1.f && max.y >= -1.0f);
    }

};

struct DrawContext
{
    std::vector<RenderObject> opaqueSurfaces;
    std::vector<RenderObject> transparentSurfaces;
};

class IRenderable {
  public:
    IRenderable() noexcept = default;
    ~IRenderable() = default;

    virtual void Draw(const glm::mat4 &topMatrix, DrawContext &ctx) = 0;
};

class LoadedGLTF final : public IRenderable
{
  public:
    LoadedGLTF() = default;
    ~LoadedGLTF()
    {
        this->ClearAll();
    };

    void Draw(const glm::mat4 &topMatrix, DrawContext &ctx) override;

  private:
    void ClearAll();

    // storage for all the data on a given gltf file
    std::unordered_map<std::string, std::shared_ptr<MeshAsset>> meshes;
    std::unordered_map<std::string, std::shared_ptr<Node>> nodes;
    std::unordered_map<std::string, AllocatedImage> images;
    std::unordered_map<std::string, std::shared_ptr<GltfMaterial>> materials;

    // nodes that dont have a parent, for iterating through the file in tree order
    std::vector<std::shared_ptr<Node>> topNodes;

    std::vector<VkSampler> samplers;

    DescriptorAllocatorGrowable descriptorPool;

    VulkanVertexBuffer materialDataBuffer;

    IRenderer *creator;

};
