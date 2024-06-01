/*! \file MaterialDefinitions.hpp
    \author Kyn21kx
    \date 2024-05-30
    \brief Contains definitions for a material object to be rendered in GPU
*/

#pragma once
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

enum class EMaterialPass : uint8_t
{
    MainColor,
    Transparent,
    Other
};
struct MaterialPipeline
{
    VkPipeline pipeline;
    VkPipelineLayout layout;
};

struct MaterialInstance {
    MaterialPipeline *pipeline;
    VkDescriptorSet materialSet;
    EMaterialPass passType;
};

//< mat_types
//> vbuf_types
struct Vertex
{

    glm::vec3 position;
    float uv_x;
    glm::vec3 normal;
    float uv_y;
    glm::vec4 color;
};
