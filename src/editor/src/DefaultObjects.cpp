#include "DefaultObjects.hpp"
#include "Scene.hpp"
#include "Entity.hpp"


Hush::Entity::EntityId Hush::DefaultObjects::MakeDefault(Scene* activeScene, EDefaultObjectType objectType) {
	Entity::EntityId result = 0;
	switch(objectType) {

	case Hush::DefaultObjects::EntWithDirectionalLight:
		// activeScene->CreateEntityWithName("Directional Light").AddComponent<DirectionalLight>();
		break;
	}
	return result;
}

