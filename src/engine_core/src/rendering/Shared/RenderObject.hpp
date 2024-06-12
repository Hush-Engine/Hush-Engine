/*! \file RenderObject.hpp
    \author Kyn21kx
    \date 2024-05-30
    \brief Basic struct of a renderable object
*/

#pragma once
#include <vulkan/vulkan.h>
#include "MaterialDefinitions.hpp"
#include <vector>
#include <array>

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

    bool IsVisible(const glm::mat4 &viewProjection)
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
            glm::vec4 v = matrix * glm::vec4(this->bounds.origin + (corners[c] * this->bounds.extents), 1.f);

            // perspective correction
            v.x = v.x / v.w;
            v.y = v.y / v.w;
            v.z = v.z / v.w;

            min = glm::min(glm::vec3{v.x, v.y, v.z}, min);
            max = glm::max(glm::vec3{v.x, v.y, v.z}, max);
        }

        // check the clip space box is within the view
        return (min.z <= 1.f && max.z >= 0.f && min.x <= 1.f && max.x >= -1.f && min.y <= 1.f && max.y >= -1.f);
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