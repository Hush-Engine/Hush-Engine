#pragma once

#include <cstdint>

namespace Hush {
	class ScriptingHost;
	struct ScriptingSystemInfo;
	class Scene;
}

namespace Hush::SystemSelection {
	
	bool RenderSystemListWindow(Scene* scene, ScriptingHost* scriptingHost, int32_t* selectedSystemIndex);

}

