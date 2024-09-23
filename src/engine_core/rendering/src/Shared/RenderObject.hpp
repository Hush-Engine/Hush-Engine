/*! \file RenderObject.hpp
    \author Kyn21kx
    \date 2024-05-30
    \brief Basic struct of a renderable object
*/

#pragma once
#include <vulkan/vulkan.h>
#include "MaterialDefinitions.hpp"
#include <vector>

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