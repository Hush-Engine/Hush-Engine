#pragma once
#include <cstdint>
namespace Hush {

	struct ShaderBindings {

		enum class EBindingType {
			Unknown,
			PushConstant,
			InputVariable,
			UniformBuffer,
			StorageBuffer,
			Sampler,
			Texture,
			CombinedImageSampler,
			StorageImage,
			UniformTexelBuffer,
			StorageTexelBuffer,
			UniformBufferDynamic,
			StorageBufferDynamic,
			InputAttachment,
			InlineUniformBlock,
			AccelerationStructure,
			UniformBufferMember,
		};

		//NOTE: Some of these variables are not all needed for all bindings, but will be there for the applicable ones
		uint32_t bindingIndex{};
		uint32_t size{};
		uint32_t offset{};
		uint32_t stageFlags{};
		uint32_t setIndex{};
		EBindingType type = EBindingType::Unknown;
	};

}
