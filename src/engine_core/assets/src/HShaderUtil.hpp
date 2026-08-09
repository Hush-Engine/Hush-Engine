#pragma once

#include "HShader.hpp"
#include "RHI/IShaderModule.hpp"
#include <cstddef>
#include <string>
#include <vector>

namespace Hush
{

	/// Helper: convert an HShader blob (WGSL backend) to ShaderModuleDescriptors
	/// that can be passed to IGraphicsDevice::CreateShaderModule().
	/// Returns one descriptor per stage, each with its own sliced WGSL module.
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

			for (const auto &stage : bd.stages)
			{
				// Slice this stage's WGSL module out of the backend blob, guarding
				// against malformed ranges.
				if (stage.codeOffset > bd.bytecode.size() || stage.codeSize > bd.bytecode.size() - stage.codeOffset)
					continue;

				std::string wgslText(reinterpret_cast<const char *>(bd.bytecode.data() + stage.codeOffset),
									 static_cast<size_t>(stage.codeSize));

				Graphics::ShaderModuleDescriptor desc;
				desc.stage = static_cast<Graphics::EShaderStage>(stage.stage);
				desc.entryPoint = stage.entryName;
				desc.bytecode.content = Graphics::ShaderBytecode::TextContent{std::move(wgslText)};
				desc.debugName = stage.entryName;

				descriptors.push_back(std::move(desc));
			}
		}

		return descriptors;
	}

} // namespace Hush
