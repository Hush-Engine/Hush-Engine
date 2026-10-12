#pragma once

#include <cstdint>
#include <unordered_set>
#include <vector>

namespace Hush
{
	class Scene;
} // namespace Hush

namespace Hush::SystemSelection
{
	struct State
	{
		std::unordered_set<uint64_t> checked; // typeId.id of each checked system

		void Clear() { checked.clear(); }
	};

	bool RenderSystemListWindow(Scene *scene, State *state);

} // namespace Hush::SystemSelection
