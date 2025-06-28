/*! \file ResourceManager.hpp
	\author Kyn21kx
	\date 2025-06-27
	\brief A resource manager for the editor, not a part of the engine core because we need to know the type of each resource and that introduces dependencies
*/

#pragma once

#include <cstdint>

namespace Hush
{
	class ResourceManager {
	public:
		
	private:
		std::unordered_map<uint64_t, ImageTexture>
	};
}
