#pragma once

#include "Entity.hpp"

namespace Hush::DefaultObjects {

	enum EDefaultObjectType {
		EntWithDirectionalLight
	};

	Entity::EntityId MakeDefault(Scene* activeScene, EDefaultObjectType objectType);	
}

