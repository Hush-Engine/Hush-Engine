/*! \file MaterialDefinitions.hpp
    \author Kyn21kx
    \date 2024-05-30
    \brief Contains definitions for a material object to be rendered in GPU
*/

#pragma once
#include "rendering/Vulkan/VkTypes.hpp"
#include <vulkan/vulkan.h>
#include <glm/glm.hpp>

namespace Hush {
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

	struct GLTFMaterial {
				MaterialInstance data;
	};

	//< mat_types
	//> vbuf_types
	struct Vertex
	{

	    float uvX;
	    float uvY;
	    glm::vec3 position;
	    glm::vec3 normal;
	    glm::vec4 color;
	};
}
