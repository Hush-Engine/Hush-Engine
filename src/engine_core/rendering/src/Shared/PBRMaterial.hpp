#pragma once
#include "Assertions.hpp"
#include <glm/glm.hpp>
/// Definition of the default PBR material the engine uses to render 3D meshes by default

namespace Hush
{
	enum class EPBRMaterialFlags : uint32_t
	{
		None = 0,
		UseNormals = 1,
		DebugNormals = 2,
		DebugLighting = 4
	};

	/// @brief Matches SceneData in mesh.slang, per-frame camera/scene state
	struct SceneData
	{
		glm::mat4 view;
		glm::mat4 proj;
		glm::mat4 viewproj;
		glm::vec4 ambientColor;
		glm::vec4 sunlightDirection; // .w = sun power
		glm::vec4 sunlightColor;
	};

	/// @brief Uniform descriptor of the material that matches the PBR's material definition on the mesh shader
	struct PBRMaterialData
	{
		glm::vec4 colorFactors;
		glm::vec4 metalRoughFactors;
		glm::vec4 emissionFactors;
		float alphaCutoff = 0.5f;
		EPBRMaterialFlags optionFlags = EPBRMaterialFlags::None;
		uint8_t padding[8];
	};

	HUSH_STATIC_ASSERT(sizeof(PBRMaterialData) == 64,
					   "PBRMaterialData should be 64 bytes to comply with shader-side uniform buffer size!");
} // namespace Hush
