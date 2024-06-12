/*! \file VkOperations.hpp
    \author Kyn21kx
    \date 2024-06-01
    \brief Utility operations for Vulkan
*/

#pragma once
#include <vulkan/vulkan.h>
#include <string_view>
#include <fstream>
#include <vector>
#include <cstdint>

namespace Hush {
    class VkOperations
    {
      public:
        static bool LoadShaderModule(const std::string_view &filePath, VkDevice device, VkShaderModule *outShaderModule)
        {
            // open the file. With cursor at the end
            std::ifstream file(filePath.data(), std::ios::ate | std::ios::binary);

            if (!file.is_open())
            {
                return false;
            }

            // find what the size of the file is by looking up the location of the cursor
            // because the cursor is at the end, it gives the size directly in bytes
            uint32_t fileSize = static_cast<uint32_t>(file.tellg());

            // spirv expects the buffer to be on uint32, so make sure to reserve a int
            // vector big enough for the entire file
            std::vector<uint32_t> buffer(fileSize / sizeof(uint32_t));

            // put file cursor at beginning
            file.seekg(0);

            // load the entire file into the buffer
            auto *fileData = reinterpret_cast<char *>(
                buffer.data()); // We downsize this, but idk, this is how it expects us to use this
            file.read(fileData, fileSize);

            // now that the file is loaded into the buffer, we can close it
            file.close();

            // create a new shader module, using the buffer we loaded
            VkShaderModuleCreateInfo createInfo = {};
            createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
            createInfo.pNext = nullptr;

            // codeSize has to be in bytes, so multply the ints in the buffer by size of
            // int to know the real size of the buffer
            createInfo.codeSize = buffer.size() * sizeof(uint32_t);
            createInfo.pCode = buffer.data();

            // check that the creation goes well.
            VkShaderModule shaderModule = nullptr;
            if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS)
            {
                return false;
            }
            *outShaderModule = shaderModule;
            return true;
        }
        static std::shared_ptr<LoadedGLTF> loadGltf(VulkanEngine *engine, std::string_view filePath);
    };
}