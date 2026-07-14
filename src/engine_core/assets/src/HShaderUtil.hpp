#pragma once

#include "HShader.hpp"
#include "RHI/IShaderModule.hpp"
#include <algorithm>
#include <cctype>
#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace Hush
{

/// Helper: convert an HShader blob (WGSL backend) to ShaderModuleDescriptors
/// that can be passed to IGraphicsDevice::CreateShaderModule().
/// Returns descriptors for all stages found in the WGSL backend.
inline std::vector<Graphics::ShaderModuleDescriptor> HShaderToModuleDescriptors(const HShader &shader)
{
	std::vector<Graphics::ShaderModuleDescriptor> descriptors;

	for (size_t b = 0; b < shader.backends.size(); ++b)
	{
		if (shader.backends[b].backendType != EShaderBackend::WebGPU_WGSL)
			continue;

		if (b >= shader.backendData.size())
			continue;

		const auto &bd = shader.backendData[b];
		if (bd.bytecode.empty())
			continue;

		// Convert the raw bytecode to WGSL text
		std::string wgslText(reinterpret_cast<const char *>(bd.bytecode.data()), bd.bytecode.size());

		for (size_t s = 0; s < bd.entryNames.size(); ++s)
		{
			Graphics::ShaderModuleDescriptor desc;
			// Infer stage from entry name
			auto lower = bd.entryNames[s];
			std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
			if (lower.find("vertex") != std::string::npos)
				desc.stage = Graphics::EShaderStage::Vertex;
			else if (lower.find("fragment") != std::string::npos || lower.find("pixel") != std::string::npos)
				desc.stage = Graphics::EShaderStage::Fragment;
			else if (lower.find("compute") != std::string::npos)
				desc.stage = Graphics::EShaderStage::Compute;

			desc.entryPoint = bd.entryNames[s];
			desc.bytecode.content = Graphics::ShaderBytecode::TextContent{wgslText};
			desc.debugName = bd.entryNames[s];

			descriptors.push_back(std::move(desc));
		}
	}

	return descriptors;
}

} // namespace Hush
