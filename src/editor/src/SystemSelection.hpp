#pragma once

#include <vector>

namespace Hush
{
	class ScriptingHost;
	struct ScriptingSystemInfo;
	class Scene;
} // namespace Hush

namespace Hush::SystemSelection
{
	struct State
	{
		std::vector<bool> checked;

		void Clear() { std::fill(checked.begin(), checked.end(), false); }
	};

	bool RenderSystemListWindow(Scene *scene, ScriptingHost *scriptingHost, State *state);

}
