// Auto-generated file
// DO NOT EDIT

#include "bindings.hpp"
#include "HushBindings.h"

void Hush__Entity_destroy(Hush__Entity **self)
{
	Hush::Entity *selfClass = reinterpret_cast<Hush::Entity *>(*self);
	if (selfClass != nullptr)
	{
		 selfClass->~Entity();
	}
	*self = nullptr;
}

void Hush__RawQuery_destroy(Hush__RawQuery **self)
{
	Hush::RawQuery *selfClass = reinterpret_cast<Hush::RawQuery *>(*self);
	if (selfClass != nullptr)
	{
		 selfClass->~RawQuery();
	}
	*self = nullptr;
}

void Hush__RawQuery__QueryIterator_destroy(Hush__RawQuery__QueryIterator **self)
{
	Hush::RawQuery::QueryIterator *selfClass = reinterpret_cast<Hush::RawQuery::QueryIterator *>(*self);
	if (selfClass != nullptr)
	{
		 selfClass->~QueryIterator();
	}
	*self = nullptr;
}

void Hush__OpaqueQueryDescriptor_destroy(Hush__OpaqueQueryDescriptor **self)
{
	Hush::OpaqueQueryDescriptor *selfClass = reinterpret_cast<Hush::OpaqueQueryDescriptor *>(*self);
	if (selfClass != nullptr)
	{
		 selfClass->~OpaqueQueryDescriptor();
	}
	*self = nullptr;
}

void Hush__Transform_destroy(Hush__Transform **self)
{
	Hush::Transform *selfClass = reinterpret_cast<Hush::Transform *>(*self);
	if (selfClass != nullptr)
	{
		 selfClass->~Transform();
	}
	*self = nullptr;
}

unsigned long long Hush__Entity__RegisterComponentRaw(Hush__Entity *self, const Hush__ComponentTraits__ComponentInfo * desc)
{
	auto selfClass = reinterpret_cast<Hush::Entity*>(self);
	auto result______ = selfClass->RegisterComponentRaw(*reinterpret_cast<const Hush::ComponentTraits::ComponentInfo *>(desc));
	return *reinterpret_cast<unsigned long long*>(&result______);
}

void * Hush__Entity__AddComponentRaw(Hush__Entity *self, unsigned long long componentId)
{
	auto selfClass = reinterpret_cast<Hush::Entity*>(self);
	auto result______ = selfClass->AddComponentRaw(componentId);
	return reinterpret_cast<void *>(result______);
}

void * Hush__Entity__GetComponentRaw(Hush__Entity *self, unsigned long long componentId)
{
	auto selfClass = reinterpret_cast<Hush::Entity*>(self);
	auto result______ = selfClass->GetComponentRaw(componentId);
	return reinterpret_cast<void *>(result______);
}

_Bool Hush__Entity__HasComponentRaw(Hush__Entity *self, unsigned long long componentId)
{
	auto selfClass = reinterpret_cast<Hush::Entity*>(self);
	auto result______ = selfClass->HasComponentRaw(componentId);
	return *reinterpret_cast<_Bool*>(&result______);
}

void * Hush__Entity__EmplaceComponentRaw(Hush__Entity *self, unsigned long long componentId, bool * isNew)
{
	auto selfClass = reinterpret_cast<Hush::Entity*>(self);
	auto result______ = selfClass->EmplaceComponentRaw(componentId, *reinterpret_cast<bool *>(isNew));
	return reinterpret_cast<void *>(result______);
}

_Bool Hush__Entity__RemoveComponentRaw(Hush__Entity *self, unsigned long long componentId)
{
	auto selfClass = reinterpret_cast<Hush::Entity*>(self);
	auto result______ = selfClass->RemoveComponentRaw(componentId);
	return *reinterpret_cast<_Bool*>(&result______);
}

void Hush__Entity__SetComponentActiveRaw(Hush__Entity *self, unsigned long long componentId, _Bool active)
{
	auto selfClass = reinterpret_cast<Hush::Entity*>(self);
	selfClass->SetComponentActiveRaw(componentId, active);
}

void Hush__Entity__AddChild(Hush__Entity *self, const Hush__Entity * child)
{
	auto selfClass = reinterpret_cast<Hush::Entity*>(self);
	selfClass->AddChild(*reinterpret_cast<const Hush::Entity *>(child));
}

int Hush__Entity__GetChildCount(Hush__Entity *self)
{
	auto selfClass = reinterpret_cast<Hush::Entity*>(self);
	auto result______ = selfClass->GetChildCount();
	return *reinterpret_cast<int*>(&result______);
}

void Hush__Entity__AddRelationship(Hush__Entity *self, const Hush__Entity * relationship, const Hush__Entity * target)
{
	auto selfClass = reinterpret_cast<Hush::Entity*>(self);
	selfClass->AddRelationship(*reinterpret_cast<const Hush::Entity *>(relationship), *reinterpret_cast<const Hush::Entity *>(target));
}

unsigned long long Hush__Entity__GetId(Hush__Entity *self)
{
	auto selfClass = reinterpret_cast<Hush::Entity*>(self);
	auto result______ = selfClass->GetId();
	return *reinterpret_cast<unsigned long long*>(&result______);
}

_Bool Hush__RawQuery__QueryIterator__Next(Hush__RawQuery__QueryIterator *self)
{
	auto selfClass = reinterpret_cast<Hush::RawQuery::QueryIterator*>(self);
	auto result______ = selfClass->Next();
	return *reinterpret_cast<_Bool*>(&result______);
}

void Hush__RawQuery__QueryIterator__Skip(Hush__RawQuery__QueryIterator *self)
{
	auto selfClass = reinterpret_cast<Hush::RawQuery::QueryIterator*>(self);
	selfClass->Skip();
}

_Bool Hush__RawQuery__QueryIterator__Finished(Hush__RawQuery__QueryIterator *self)
{
	auto selfClass = reinterpret_cast<Hush::RawQuery::QueryIterator*>(self);
	auto result______ = selfClass->Finished();
	return *reinterpret_cast<_Bool*>(&result______);
}

unsigned long long Hush__RawQuery__QueryIterator__Size(Hush__RawQuery__QueryIterator *self)
{
	auto selfClass = reinterpret_cast<Hush::RawQuery::QueryIterator*>(self);
	auto result______ = selfClass->Size();
	return *reinterpret_cast<unsigned long long*>(&result______);
}

void * Hush__RawQuery__QueryIterator__GetComponentAt(Hush__RawQuery__QueryIterator *self, signed char index, unsigned long long size)
{
	auto selfClass = reinterpret_cast<Hush::RawQuery::QueryIterator*>(self);
	auto result______ = selfClass->GetComponentAt(index, size);
	return reinterpret_cast<void *>(result______);
}

unsigned long long Hush__RawQuery__QueryIterator__GetEntityAt(Hush__RawQuery__QueryIterator *self, unsigned long long index)
{
	auto selfClass = reinterpret_cast<Hush::RawQuery::QueryIterator*>(self);
	auto result______ = selfClass->GetEntityAt(index);
	return *reinterpret_cast<unsigned long long*>(&result______);
}

Hush__Scene * Hush__RawQuery__GetScene(Hush__RawQuery *self)
{
	auto selfClass = reinterpret_cast<Hush::RawQuery*>(self);
	auto result______ = selfClass->GetScene();
	return reinterpret_cast<Hush__Scene *>(result______);
}

Hush__RawQuery__QueryIterator Hush__RawQuery__GetIterator(Hush__RawQuery *self)
{
	auto selfClass = reinterpret_cast<Hush::RawQuery*>(self);
	auto result______ = selfClass->GetIterator();
	std::aligned_storage_t<sizeof(Hush__RawQuery__QueryIterator)> resultStorage_____;
	auto *resultPtr = reinterpret_cast<decltype(result______)*>(&resultStorage_____);
	new (resultPtr) decltype(result______)(std::move(result______));
	return *reinterpret_cast<Hush__RawQuery__QueryIterator*>(resultPtr);
}

void Hush__impl__QueryBuilderImpl__WithRelationship(unsigned char * queryDesc, unsigned char * termCountRef, const Hush__Entity * relationship)
{
	Hush::impl::QueryBuilderImpl::WithRelationship(reinterpret_cast<unsigned char *>(queryDesc), reinterpret_cast<unsigned char *>(termCountRef), *reinterpret_cast<const Hush::Entity *>(relationship));
}

void Hush__impl__QueryBuilderImpl__WithTerm(unsigned char * queryDesc, unsigned char * termCountRef, unsigned long long term)
{
	Hush::impl::QueryBuilderImpl::WithTerm(reinterpret_cast<unsigned char *>(queryDesc), reinterpret_cast<unsigned char *>(termCountRef), term);
}

void Hush__impl__QueryBuilderImpl__InitDescriptor(unsigned char * queryDesc, unsigned long long *componentsData, const size_t componentsSize)
{
	auto componentsData__ = std::span<unsigned long long>(reinterpret_cast<unsigned long long*>(componentsData), componentsSize);
	Hush::impl::QueryBuilderImpl::InitDescriptor(reinterpret_cast<unsigned char *>(queryDesc), componentsData__);
}

Hush__RawQuery Hush__impl__QueryBuilderImpl__InitQuery(Hush__Scene * scene, const unsigned char * queryDesc)
{
	auto result______ = Hush::impl::QueryBuilderImpl::InitQuery(reinterpret_cast<Hush::Scene *>(scene), reinterpret_cast<const unsigned char *>(queryDesc));
	std::aligned_storage_t<sizeof(Hush__RawQuery)> resultStorage_____;
	auto *resultPtr = reinterpret_cast<decltype(result______)*>(&resultStorage_____);
	new (resultPtr) decltype(result______)(std::move(result______));
	return *reinterpret_cast<Hush__RawQuery*>(resultPtr);
}

unsigned char * Hush__OpaqueQueryDescriptor__data(Hush__OpaqueQueryDescriptor *self)
{
	auto selfClass = reinterpret_cast<Hush::OpaqueQueryDescriptor*>(self);
	auto result______ = selfClass->data();
	return reinterpret_cast<unsigned char *>(result______);
}

void Hush__Scene__RemoveSystem(Hush__Scene *self, char *nameData, const size_t nameSize)
{
	auto selfClass = reinterpret_cast<Hush::Scene*>(self);
	auto nameData__ = std::string_view(reinterpret_cast<char*>(nameData), nameSize);
	selfClass->RemoveSystem(nameData__);
}

Hush__Entity Hush__Scene__CreateEntity(Hush__Scene *self)
{
	auto selfClass = reinterpret_cast<Hush::Scene*>(self);
	auto result______ = selfClass->CreateEntity();
	return *reinterpret_cast<Hush__Entity*>(&result______);
}

Hush__Entity Hush__Scene__CreateEntityWithName(Hush__Scene *self, char *nameData, const size_t nameSize)
{
	auto selfClass = reinterpret_cast<Hush::Scene*>(self);
	auto nameData__ = std::string_view(reinterpret_cast<char*>(nameData), nameSize);
	auto result______ = selfClass->CreateEntityWithName(nameData__);
	return *reinterpret_cast<Hush__Entity*>(&result______);
}

void Hush__Scene__RegisterComponentId(Hush__Scene *self, char *nameData, const size_t nameSize, unsigned long long id)
{
	auto selfClass = reinterpret_cast<Hush::Scene*>(self);
	auto nameData__ = std::string_view(reinterpret_cast<char*>(nameData), nameSize);
	selfClass->RegisterComponentId(nameData__, id);
}

Hush__Entity Hush__Scene__EntityFromIdUnchecked(Hush__Scene *self, unsigned long long id)
{
	auto selfClass = reinterpret_cast<Hush::Scene*>(self);
	auto result______ = selfClass->EntityFromIdUnchecked(id);
	return *reinterpret_cast<Hush__Entity*>(&result______);
}

unsigned long long Hush__Scene__RegisterComponentRaw(Hush__Scene *self, const Hush__ComponentTraits__ComponentInfo * desc)
{
	auto selfClass = reinterpret_cast<Hush::Scene*>(self);
	auto result______ = selfClass->RegisterComponentRaw(*reinterpret_cast<const Hush::ComponentTraits::ComponentInfo *>(desc));
	return *reinterpret_cast<unsigned long long*>(&result______);
}

unsigned long long Hush__Scene__Lookup(Hush__Scene *self, char *tagData, const size_t tagSize)
{
	auto selfClass = reinterpret_cast<Hush::Scene*>(self);
	auto tagData__ = std::string_view(reinterpret_cast<char*>(tagData), tagSize);
	auto result______ = selfClass->Lookup(tagData__);
	return *reinterpret_cast<unsigned long long*>(&result______);
}

Hush__RawQuery Hush__Scene__CreateRawQuery(Hush__Scene *self, unsigned long long *componentsData, const size_t componentsSize, enum Hush__RawQuery__ECacheMode cacheMode)
{
	auto selfClass = reinterpret_cast<Hush::Scene*>(self);
	auto componentsData__ = std::span<unsigned long long>(reinterpret_cast<unsigned long long*>(componentsData), componentsSize);
	auto result______ = selfClass->CreateRawQuery(componentsData__, static_cast<enum Hush::RawQuery::ECacheMode>(cacheMode));
	std::aligned_storage_t<sizeof(Hush__RawQuery)> resultStorage_____;
	auto *resultPtr = reinterpret_cast<decltype(result______)*>(&resultStorage_____);
	new (resultPtr) decltype(result______)(std::move(result______));
	return *reinterpret_cast<Hush__RawQuery*>(resultPtr);
}

Hush__Scene * Hush__HushEngine__GetScene(Hush__HushEngine *self)
{
	auto selfClass = reinterpret_cast<Hush::HushEngine*>(self);
	auto result______ = selfClass->GetScene();
	return reinterpret_cast<Hush__Scene *>(result______);
}

void Hush__Transform__SetPosition(Hush__Transform *self, Vector3 position)
{
	auto selfClass = reinterpret_cast<Hush::Transform*>(self);
	selfClass->SetPosition(position);
}

_Bool Hush__InputManager__IsKeyDown(enum Hush__EKeyCode key)
{
	auto result______ = Hush::InputManager::IsKeyDown(static_cast<enum Hush::EKeyCode>(key));
	return *reinterpret_cast<_Bool*>(&result______);
}

_Bool Hush__InputManager__IsKeyDownThisFrame(enum Hush__EKeyCode key)
{
	auto result______ = Hush::InputManager::IsKeyDownThisFrame(static_cast<enum Hush::EKeyCode>(key));
	return *reinterpret_cast<_Bool*>(&result______);
}

_Bool Hush__InputManager__IsKeyUp(enum Hush__EKeyCode key)
{
	auto result______ = Hush::InputManager::IsKeyUp(static_cast<enum Hush::EKeyCode>(key));
	return *reinterpret_cast<_Bool*>(&result______);
}

_Bool Hush__InputManager__IsKeyHeld(enum Hush__EKeyCode key)
{
	auto result______ = Hush::InputManager::IsKeyHeld(static_cast<enum Hush::EKeyCode>(key));
	return *reinterpret_cast<_Bool*>(&result______);
}

_Bool Hush__InputManager__GetMouseButtonPressed(enum Hush__EMouseButton button)
{
	auto result______ = Hush::InputManager::GetMouseButtonPressed(static_cast<enum Hush::EMouseButton>(button));
	return *reinterpret_cast<_Bool*>(&result______);
}

_Bool Hush__InputManager__FetchCharThisFrame(char * outChar)
{
	auto result______ = Hush::InputManager::FetchCharThisFrame(reinterpret_cast<char *>(outChar));
	return *reinterpret_cast<_Bool*>(&result______);
}

void Hush__InputManager__SetCursorLock(enum Hush__ECursorLockMode lockMode)
{
	Hush::InputManager::SetCursorLock(static_cast<enum Hush::ECursorLockMode>(lockMode));
}

#ifdef HUSH_STATIC_BINDING
HushFuncPtrTable HUSH_FUNCPTR_TABLE = {
	Hush__Entity__RegisterComponentRaw,
	Hush__Entity__AddComponentRaw,
	Hush__Entity__GetComponentRaw,
	Hush__Entity__HasComponentRaw,
	Hush__Entity__EmplaceComponentRaw,
	Hush__Entity__RemoveComponentRaw,
	Hush__Entity__SetComponentActiveRaw,
	Hush__Entity__AddChild,
	Hush__Entity__GetChildCount,
	Hush__Entity__AddRelationship,
	Hush__Entity__GetId,
	Hush__RawQuery__QueryIterator__Next,
	Hush__RawQuery__QueryIterator__Skip,
	Hush__RawQuery__QueryIterator__Finished,
	Hush__RawQuery__QueryIterator__Size,
	Hush__RawQuery__QueryIterator__GetComponentAt,
	Hush__RawQuery__QueryIterator__GetEntityAt,
	Hush__RawQuery__GetScene,
	Hush__RawQuery__GetIterator,
	Hush__impl__QueryBuilderImpl__WithRelationship,
	Hush__impl__QueryBuilderImpl__WithTerm,
	Hush__impl__QueryBuilderImpl__InitDescriptor,
	Hush__impl__QueryBuilderImpl__InitQuery,
	Hush__OpaqueQueryDescriptor__data,
	Hush__Scene__RemoveSystem,
	Hush__Scene__CreateEntity,
	Hush__Scene__CreateEntityWithName,
	Hush__Scene__RegisterComponentId,
	Hush__Scene__EntityFromIdUnchecked,
	Hush__Scene__RegisterComponentRaw,
	Hush__Scene__Lookup,
	Hush__Scene__CreateRawQuery,
	Hush__HushEngine__GetScene,
	Hush__Transform__SetPosition,
	Hush__InputManager__IsKeyDown,
	Hush__InputManager__IsKeyDownThisFrame,
	Hush__InputManager__IsKeyUp,
	Hush__InputManager__IsKeyHeld,
	Hush__InputManager__GetMouseButtonPressed,
	Hush__InputManager__FetchCharThisFrame,
	Hush__InputManager__SetCursorLock,
};
#endif
