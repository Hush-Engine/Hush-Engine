/*! \file VkOperations.hpp
    \author Kyn21kx
    \date 2024-06-01
    \brief Utility operations for Vulkan
*/

#pragma once
#include "log/Logger.hpp"
#include "rendering/Renderer.hpp"
#include "rendering/Shared/RenderObject.hpp"
#include <vulkan/vulkan.h>
#include <string_view>
#include <fstream>
#include <vector>
#include <cstdint>
#include <fastgltf/parser.hpp>

namespace Hush {
    class VkOperations
    {
      public:
        static bool LoadShaderModule(const std::string_view &filePath, VkDevice device,
                                     VkShaderModule *outShaderModule);

        static std::shared_ptr<LoadedGLTF> LoadGltf(IRenderer *baseRenderer, const std::string_view &filePath);

        static bool LoadImageToRender(IRenderer *baseRenderer, fastgltf::Asset &asset, fastgltf::Image &image, AllocatedImage* outImage);

      private:
        static VkFilter ExtractFilter(fastgltf::Filter filter) {
            switch (filter)
            {
            // nearest samplers
            case fastgltf::Filter::Nearest:
            case fastgltf::Filter::NearestMipMapNearest:
            case fastgltf::Filter::NearestMipMapLinear:
                return VK_FILTER_NEAREST;

            // linear samplers
            case fastgltf::Filter::Linear:
            case fastgltf::Filter::LinearMipMapNearest:
            case fastgltf::Filter::LinearMipMapLinear:
            default:
                return VK_FILTER_LINEAR;
            }
        }

        static VkSamplerMipmapMode ExtractMipMapMode(fastgltf::Filter filter) {
            switch (filter)
            {
            case fastgltf::Filter::NearestMipMapNearest:
            case fastgltf::Filter::LinearMipMapNearest:
                return VK_SAMPLER_MIPMAP_MODE_NEAREST;

            case fastgltf::Filter::NearestMipMapLinear:
            case fastgltf::Filter::LinearMipMapLinear:
            default:
                return VK_SAMPLER_MIPMAP_MODE_LINEAR;
            }
        }
    };
}
