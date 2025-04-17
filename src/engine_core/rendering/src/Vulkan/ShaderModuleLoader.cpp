#include "Vulkan/ShaderModuleLoader.hpp"
#include "Vulkan/VulkanPipelineBuilder.hpp"
#include "crypto/Hashing.hpp"
#include <cstdint>

bool Hush::ShaderModuleLoader::LoadShaderModule(const std::string_view &filePath, VkDevice device, VkShaderModule *outShaderModule, std::vector<uint32_t> *outBuffer) {
	uint32_t hashEntry = Hush::Hashing::Fnv1a(filePath);
	if (this->m_loadedShaderModules.find(hashEntry) == this->m_loadedShaderModules.end()) {
		bool inserted = VulkanHelper::LoadShaderModule(filePath, device, outShaderModule, outBuffer);
		if (inserted) {
			this->m_loadedShaderModules.insert_or_assign(hashEntry, *outShaderModule);
		}
		return inserted;
	}
	VkShaderModule module = this->m_loadedShaderModules.at(hashEntry);
	*outShaderModule = module;
	return module != nullptr;
}
