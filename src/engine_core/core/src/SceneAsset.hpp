#pragma once
// This is in the core module even though it's a resource because
// the core engine should still know how to convert it to a Hush::Scene*

#include "Entity.hpp"
#include "serialization/SerializedEntity.hpp"
#include <cstdint>
#include <vector>


#include <reflection/Type.hpp>
#include <serialization/Serialization.hpp>
#include <serialization/Deserialization.hpp>
#include <type_traits>
#include <Hushgen.hpp>

#if __has_include("SceneAsset.hushgen.hpp") && !defined(HUSH_HEADER_PARSING)
#include "SceneAsset.hushgen.hpp"
#endif
namespace Hush
{

	// TODO: This is our first implementation of scene assets, it is highly subject to change
	//  Also, this class allocates like crazy, so, watch out for that
	class [[hush::reflect]] SceneAsset
	{
	static constexpr uint8_t HSCENE_V_MAJOR = 0;
	static constexpr uint8_t HSCENE_V_MINOR = 1;
		HUSH_GENERATED_BODY
	public:

		// struct SerializedSystem
		// {
		// 	std::string type;
		// 	int32_t order;
		// };

		// Some metadata
		[[hush::property]]
		uint8_t versionMajor = HSCENE_V_MAJOR;
		[[hush::property]]
		uint8_t versionMinor = HSCENE_V_MINOR;

		/// @brief UUID that corresponds to this scene asset
		[[hush::property]]
		uint64_t sceneId;

		// entities
		[[hush::property]]
		std::vector<SerializedEntity> entities;
		std::string sceneJson;
	};

} // namespace Hush
