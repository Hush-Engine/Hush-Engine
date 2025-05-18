#pragma once

#include <cstdint>
#include <string_view>
#include <unordered_map>
#include <vulkan/vulkan_core.h>
namespace Hush
{

	class ShaderModuleLoader
	{

	public:
		bool LoadShaderModule(const std::string_view &filePath, VkDevice device, VkShaderModule *outShaderModule,
							  std::vector<uint32_t> *outBuffer = nullptr);

	private:
		std::unordered_map<uint32_t, VkShaderModule> m_loadedShaderModules;
	};
} // namespace Hush
