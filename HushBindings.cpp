// Auto-generated file
// DO NOT EDIT
// NOLINTBEGIN

#include "bindings.hpp"
#include "HushBindings.h"

void Hush__Reflection__TypeId_destroy(Hush__Reflection__TypeId **self)
{
	Hush::Reflection::TypeId *selfClass = reinterpret_cast<Hush::Reflection::TypeId *>(*self);
	if (selfClass != nullptr)
	{
		selfClass->~TypeId();
	}
	*self = nullptr;
}

void Hush__Reflection__ForeignVariant_destroy(Hush__Reflection__ForeignVariant **self)
{
	Hush::Reflection::ForeignVariant *selfClass = reinterpret_cast<Hush::Reflection::ForeignVariant *>(*self);
	if (selfClass != nullptr)
	{
		selfClass->~ForeignVariant();
	}
	*self = nullptr;
}

void Hush__Reflection__FieldInfo_destroy(Hush__Reflection__FieldInfo **self)
{
	Hush::Reflection::FieldInfo *selfClass = reinterpret_cast<Hush::Reflection::FieldInfo *>(*self);
	if (selfClass != nullptr)
	{
		selfClass->~FieldInfo();
	}
	*self = nullptr;
}

void Hush__Reflection__TypeInfo_destroy(Hush__Reflection__TypeInfo **self)
{
	Hush::Reflection::TypeInfo *selfClass = reinterpret_cast<Hush::Reflection::TypeInfo *>(*self);
	if (selfClass != nullptr)
	{
		selfClass->~TypeInfo();
	}
	*self = nullptr;
}

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

void Hush__Camera_destroy(Hush__Camera **self)
{
	Hush::Camera *selfClass = reinterpret_cast<Hush::Camera *>(*self);
	if (selfClass != nullptr)
	{
		selfClass->~Camera();
	}
	*self = nullptr;
}

void Hush__Graphics__Material3D_destroy(Hush__Graphics__Material3D **self)
{
	Hush::Graphics::Material3D *selfClass = reinterpret_cast<Hush::Graphics::Material3D *>(*self);
	if (selfClass != nullptr)
	{
		selfClass->~Material3D();
	}
	*self = nullptr;
}

void Hush__Image_destroy(Hush__Image **self)
{
	Hush::Image *selfClass = reinterpret_cast<Hush::Image *>(*self);
	if (selfClass != nullptr)
	{
		selfClass->~Image();
	}
	*self = nullptr;
}

void Hush__TextureComponent_destroy(Hush__TextureComponent **self)
{
	Hush::TextureComponent *selfClass = reinterpret_cast<Hush::TextureComponent *>(*self);
	if (selfClass != nullptr)
	{
		selfClass->~TextureComponent();
	}
	*self = nullptr;
}

void Hush__MeshReference_destroy(Hush__MeshReference **self)
{
	Hush::MeshReference *selfClass = reinterpret_cast<Hush::MeshReference *>(*self);
	if (selfClass != nullptr)
	{
		selfClass->~MeshReference();
	}
	*self = nullptr;
}

void Hush__InspectableComponent_destroy(Hush__InspectableComponent **self)
{
	Hush::InspectableComponent *selfClass = reinterpret_cast<Hush::InspectableComponent *>(*self);
	if (selfClass != nullptr)
	{
		selfClass->~InspectableComponent();
	}
	*self = nullptr;
}

void Hush__Reflection__TypeInfo__SetName(Hush__Reflection__TypeInfo *self, const char *nameData, const size_t nameSize)
{
	auto selfClass = reinterpret_cast<Hush::Reflection::TypeInfo *>(self);
	auto nameData__ = std::basic_string_view<char>(reinterpret_cast<const char *>(nameData), nameSize);
	selfClass->SetName(nameData__);
}

void Hush__Reflection__TypeInfo__SetSize(Hush__Reflection__TypeInfo *self, size_t size)
{
	auto selfClass = reinterpret_cast<Hush::Reflection::TypeInfo *>(self);
	selfClass->SetSize(size);
}

void Hush__Reflection__TypeInfo__SetAlignment(Hush__Reflection__TypeInfo *self, size_t alignment)
{
	auto selfClass = reinterpret_cast<Hush::Reflection::TypeInfo *>(self);
	selfClass->SetAlignment(alignment);
}

Hush__Reflection__ERegisterClassError Hush__Reflection__ReflectionDB__RegisterForeignClass(
	Hush__Reflection__ReflectionDB *self, const char *nameData, const size_t nameSize, size_t size, size_t alignment,
	Hush__ModuleHandle module)
{
	auto selfClass = reinterpret_cast<Hush::Reflection::ReflectionDB *>(self);
	auto nameData__ = std::basic_string_view<char>(reinterpret_cast<const char *>(nameData), nameSize);
	auto result______ = selfClass->RegisterForeignClass(nameData__, size, alignment, module);
	return static_cast<Hush__Reflection__ERegisterClassError>(result______);
}

bool Hush__Reflection__ReflectionDB__AddFieldToForeignClass(Hush__Reflection__ReflectionDB *self,
															Hush__Reflection__TypeId classType,
															const char *fieldNameData, const size_t fieldNameSize,
															Hush__Reflection__TypeId type, uint64_t offset)
{
	auto selfClass = reinterpret_cast<Hush::Reflection::ReflectionDB *>(self);
	auto fieldNameData__ = std::basic_string_view<char>(reinterpret_cast<const char *>(fieldNameData), fieldNameSize);
	auto result______ = selfClass->AddFieldToForeignClass(
		reinterpret_cast<const Hush::Reflection::TypeId &>(classType), fieldNameData__,
		reinterpret_cast<const Hush::Reflection::TypeId &>(type), offset);
	return *reinterpret_cast<bool *>(&result______);
}

Hush__Entity_EntityId Hush__Entity__RegisterComponentRaw(Hush__Entity *self,
														 const Hush__ComponentTraits__ComponentInfo *desc)
{
	auto selfClass = reinterpret_cast<Hush::Entity *>(self);
	auto result______ =
		selfClass->RegisterComponentRaw(*reinterpret_cast<const Hush::ComponentTraits::ComponentInfo *>(desc));
	return *reinterpret_cast<Hush__Entity_EntityId *>(&result______);
}

void *Hush__Entity__AddComponentRaw(Hush__Entity *self, Hush__Entity_EntityId componentId)
{
	auto selfClass = reinterpret_cast<Hush::Entity *>(self);
	auto result______ = selfClass->AddComponentRaw(componentId);
	return reinterpret_cast<void *>(result______);
}

void Hush__Entity__AddId(Hush__Entity *self, Hush__Entity_EntityId componentId)
{
	auto selfClass = reinterpret_cast<Hush::Entity *>(self);
	selfClass->AddId(componentId);
}

void *Hush__Entity__GetComponentRaw(Hush__Entity *self, Hush__Entity_EntityId componentId)
{
	auto selfClass = reinterpret_cast<Hush::Entity *>(self);
	auto result______ = selfClass->GetComponentRaw(componentId);
	return reinterpret_cast<void *>(result______);
}

const void *Hush__Entity__GetComponentConstRaw(Hush__Entity *self, Hush__Entity_EntityId componentId)
{
	auto selfClass = reinterpret_cast<Hush::Entity *>(self);
	auto result______ = selfClass->GetComponentConstRaw(componentId);
	return reinterpret_cast<const void *>(result______);
}

bool Hush__Entity__HasComponentRaw(Hush__Entity *self, Hush__Entity_EntityId componentId)
{
	auto selfClass = reinterpret_cast<Hush::Entity *>(self);
	auto result______ = selfClass->HasComponentRaw(componentId);
	return *reinterpret_cast<bool *>(&result______);
}

void *Hush__Entity__EmplaceComponentRaw(Hush__Entity *self, Hush__Entity_EntityId componentId, size_t componentSize,
										bool *isNew)
{
	auto selfClass = reinterpret_cast<Hush::Entity *>(self);
	auto result______ = selfClass->EmplaceComponentRaw(componentId, componentSize, *reinterpret_cast<bool *>(isNew));
	return reinterpret_cast<void *>(result______);
}

bool Hush__Entity__RemoveComponentRaw(Hush__Entity *self, Hush__Entity_EntityId componentId)
{
	auto selfClass = reinterpret_cast<Hush::Entity *>(self);
	auto result______ = selfClass->RemoveComponentRaw(componentId);
	return *reinterpret_cast<bool *>(&result______);
}

void Hush__Entity__SetComponentActiveRaw(Hush__Entity *self, Hush__Entity_EntityId componentId, bool active)
{
	auto selfClass = reinterpret_cast<Hush::Entity *>(self);
	selfClass->SetComponentActiveRaw(componentId, active);
}

void Hush__Entity__AddChild(Hush__Entity *self, const Hush__Entity *child)
{
	auto selfClass = reinterpret_cast<Hush::Entity *>(self);
	selfClass->AddChild(*reinterpret_cast<const Hush::Entity *>(child));
}

int32_t Hush__Entity__GetChildCount(Hush__Entity *self)
{
	auto selfClass = reinterpret_cast<Hush::Entity *>(self);
	auto result______ = selfClass->GetChildCount();
	return *reinterpret_cast<int32_t *>(&result______);
}

void Hush__Entity__AddRelationship(Hush__Entity *self, const Hush__Entity *relationship, const Hush__Entity *target)
{
	auto selfClass = reinterpret_cast<Hush::Entity *>(self);
	selfClass->AddRelationship(*reinterpret_cast<const Hush::Entity *>(relationship),
							   *reinterpret_cast<const Hush::Entity *>(target));
}

Hush__Entity_EntityId Hush__Entity__GetId(Hush__Entity *self)
{
	auto selfClass = reinterpret_cast<Hush::Entity *>(self);
	auto result______ = selfClass->GetId();
	return *reinterpret_cast<Hush__Entity_EntityId *>(&result______);
}

void Hush__Entity__SetName(Hush__Entity *self, const char *nameData, const size_t nameSize)
{
	auto selfClass = reinterpret_cast<Hush::Entity *>(self);
	auto nameData__ = std::basic_string_view<char>(reinterpret_cast<const char *>(nameData), nameSize);
	selfClass->SetName(nameData__);
}

void Hush__Entity__SetKey(Hush__Entity *self, const char *keyData, const size_t keySize)
{
	auto selfClass = reinterpret_cast<Hush::Entity *>(self);
	auto keyData__ = std::basic_string_view<char>(reinterpret_cast<const char *>(keyData), keySize);
	selfClass->SetKey(keyData__);
}

void Hush__Entity__GetKey(Hush__Entity *self, char *bufferData, const size_t bufferSize)
{
	auto selfClass = reinterpret_cast<Hush::Entity *>(self);
	auto bufferData__ = std::span<char>(reinterpret_cast<char *>(bufferData), bufferSize);
	selfClass->GetKey(bufferData__);
}

void Hush__Entity__QueryName(Hush__Entity *self, char *bufferData, const size_t bufferSize)
{
	auto selfClass = reinterpret_cast<Hush::Entity *>(self);
	auto bufferData__ = std::span<char>(reinterpret_cast<char *>(bufferData), bufferSize);
	selfClass->QueryName(bufferData__);
}

bool Hush__Entity__IsAlive(Hush__Entity *self)
{
	auto selfClass = reinterpret_cast<Hush::Entity *>(self);
	auto result______ = selfClass->IsAlive();
	return *reinterpret_cast<bool *>(&result______);
}

bool Hush__RawQuery__QueryIterator__Next(Hush__RawQuery__QueryIterator *self)
{
	auto selfClass = reinterpret_cast<Hush::RawQuery::QueryIterator *>(self);
	auto result______ = selfClass->Next();
	return *reinterpret_cast<bool *>(&result______);
}

void Hush__RawQuery__QueryIterator__Skip(Hush__RawQuery__QueryIterator *self)
{
	auto selfClass = reinterpret_cast<Hush::RawQuery::QueryIterator *>(self);
	selfClass->Skip();
}

bool Hush__RawQuery__QueryIterator__Finished(Hush__RawQuery__QueryIterator *self)
{
	auto selfClass = reinterpret_cast<Hush::RawQuery::QueryIterator *>(self);
	auto result______ = selfClass->Finished();
	return *reinterpret_cast<bool *>(&result______);
}

size_t Hush__RawQuery__QueryIterator__Size(Hush__RawQuery__QueryIterator *self)
{
	auto selfClass = reinterpret_cast<Hush::RawQuery::QueryIterator *>(self);
	auto result______ = selfClass->Size();
	return *reinterpret_cast<size_t *>(&result______);
}

void *Hush__RawQuery__QueryIterator__GetComponentAt(Hush__RawQuery__QueryIterator *self, int8_t index, size_t size)
{
	auto selfClass = reinterpret_cast<Hush::RawQuery::QueryIterator *>(self);
	auto result______ = selfClass->GetComponentAt(index, size);
	return reinterpret_cast<void *>(result______);
}

uint64_t Hush__RawQuery__QueryIterator__GetEntityAt(Hush__RawQuery__QueryIterator *self, size_t index)
{
	auto selfClass = reinterpret_cast<Hush::RawQuery::QueryIterator *>(self);
	auto result______ = selfClass->GetEntityAt(index);
	return *reinterpret_cast<uint64_t *>(&result______);
}

Hush__Scene *Hush__RawQuery__GetScene(Hush__RawQuery *self)
{
	auto selfClass = reinterpret_cast<Hush::RawQuery *>(self);
	auto result______ = selfClass->GetScene();
	return reinterpret_cast<Hush__Scene *>(result______);
}

Hush__RawQuery__QueryIterator Hush__RawQuery__GetIterator(Hush__RawQuery *self)
{
	auto selfClass = reinterpret_cast<Hush::RawQuery *>(self);
	auto result______ = selfClass->GetIterator();
	std::aligned_storage_t<sizeof(Hush__RawQuery__QueryIterator)> resultStorage_____;
	auto *resultPtr = reinterpret_cast<decltype(result______) *>(&resultStorage_____);
	new (resultPtr) decltype(result______)(std::move(result______));
	return *reinterpret_cast<Hush__RawQuery__QueryIterator *>(resultPtr);
}

void Hush__impl__QueryBuilderImpl__WithRelationship(uint8_t *queryDesc, uint8_t *termCountRef,
													const Hush__Entity *relationship)
{
	Hush::impl::QueryBuilderImpl::WithRelationship(reinterpret_cast<unsigned char *>(queryDesc),
												   reinterpret_cast<unsigned char *>(termCountRef),
												   *reinterpret_cast<const Hush::Entity *>(relationship));
}

void Hush__impl__QueryBuilderImpl__WithTerm(uint8_t *queryDesc, uint8_t *termCountRef, Hush__Entity_EntityId term)
{
	Hush::impl::QueryBuilderImpl::WithTerm(reinterpret_cast<unsigned char *>(queryDesc),
										   reinterpret_cast<unsigned char *>(termCountRef), term);
}

void Hush__impl__QueryBuilderImpl__InitDescriptor(uint8_t *queryDesc, Hush__Entity_EntityId *componentsData,
												  const size_t componentsSize)
{
	auto componentsData__ =
		std::span<unsigned long long>(reinterpret_cast<unsigned long long *>(componentsData), componentsSize);
	Hush::impl::QueryBuilderImpl::InitDescriptor(reinterpret_cast<unsigned char *>(queryDesc), componentsData__);
}

void Hush__impl__QueryBuilderImpl__Without(uint8_t *queryDesc, uint8_t *termCountRef, Hush__Entity_EntityId term)
{
	Hush::impl::QueryBuilderImpl::Without(reinterpret_cast<unsigned char *>(queryDesc),
										  reinterpret_cast<unsigned char *>(termCountRef), term);
}

void Hush__impl__QueryBuilderImpl__WithOptional(uint8_t *queryDesc, uint8_t *termCountRef, Hush__Entity_EntityId term)
{
	Hush::impl::QueryBuilderImpl::WithOptional(reinterpret_cast<unsigned char *>(queryDesc),
											   reinterpret_cast<unsigned char *>(termCountRef), term);
}

void Hush__impl__QueryBuilderImpl__IncludeTemplates(uint8_t *queryDesc, uint8_t *termCountRef)
{
	Hush::impl::QueryBuilderImpl::IncludeTemplates(reinterpret_cast<unsigned char *>(queryDesc),
												   reinterpret_cast<unsigned char *>(termCountRef));
}

void Hush__impl__QueryBuilderImpl__TemplatesOnly(uint8_t *queryDesc, uint8_t *termCountRef)
{
	Hush::impl::QueryBuilderImpl::TemplatesOnly(reinterpret_cast<unsigned char *>(queryDesc),
												reinterpret_cast<unsigned char *>(termCountRef));
}

Hush__RawQuery Hush__impl__QueryBuilderImpl__InitQuery(Hush__Scene *scene, const uint8_t *queryDesc)
{
	auto result______ = Hush::impl::QueryBuilderImpl::InitQuery(reinterpret_cast<Hush::Scene *>(scene),
																reinterpret_cast<const unsigned char *>(queryDesc));
	std::aligned_storage_t<sizeof(Hush__RawQuery)> resultStorage_____;
	auto *resultPtr = reinterpret_cast<decltype(result______) *>(&resultStorage_____);
	new (resultPtr) decltype(result______)(std::move(result______));
	return *reinterpret_cast<Hush__RawQuery *>(resultPtr);
}

uint8_t *Hush__OpaqueQueryDescriptor__data(Hush__OpaqueQueryDescriptor *self)
{
	auto selfClass = reinterpret_cast<Hush::OpaqueQueryDescriptor *>(self);
	auto result______ = selfClass->data();
	return reinterpret_cast<uint8_t *>(result______);
}

void Hush__Scene__RemoveSystem(Hush__Scene *self, const char *nameData, const size_t nameSize)
{
	auto selfClass = reinterpret_cast<Hush::Scene *>(self);
	auto nameData__ = std::basic_string_view<char>(reinterpret_cast<const char *>(nameData), nameSize);
	selfClass->RemoveSystem(nameData__);
}

Hush__Entity Hush__Scene__CreateEntity(Hush__Scene *self)
{
	auto selfClass = reinterpret_cast<Hush::Scene *>(self);
	auto result______ = selfClass->CreateEntity();
	return *reinterpret_cast<Hush__Entity *>(&result______);
}

Hush__Entity Hush__Scene__CreateEntityWithName(Hush__Scene *self, const char *nameData, const size_t nameSize)
{
	auto selfClass = reinterpret_cast<Hush::Scene *>(self);
	auto nameData__ = std::basic_string_view<char>(reinterpret_cast<const char *>(nameData), nameSize);
	auto result______ = selfClass->CreateEntityWithName(nameData__);
	return *reinterpret_cast<Hush__Entity *>(&result______);
}

Hush__Entity Hush__Scene__CreateEntityWithKey(Hush__Scene *self, const char *keyData, const size_t keySize)
{
	auto selfClass = reinterpret_cast<Hush::Scene *>(self);
	auto keyData__ = std::basic_string_view<char>(reinterpret_cast<const char *>(keyData), keySize);
	auto result______ = selfClass->CreateEntityWithKey(keyData__);
	return *reinterpret_cast<Hush__Entity *>(&result______);
}

void Hush__Scene__AddComponentObserverRaw(Hush__Scene *self, Hush__Entity_EntityId componentId, size_t componentSize,
										  Hush__EComponentObserverType observerType, Hush__ObserverCallback_t callback)
{
	auto selfClass = reinterpret_cast<Hush::Scene *>(self);
	selfClass->AddComponentObserverRaw(componentId, componentSize,
									   static_cast<Hush::EComponentObserverType>(observerType),
									   reinterpret_cast<void (*)(unsigned long long, void *)>(callback));
}

void Hush__Scene__DestroyEntity(Hush__Scene *self, Hush__Entity *entity)
{
	auto selfClass = reinterpret_cast<Hush::Scene *>(self);
	selfClass->DestroyEntity(*reinterpret_cast<Hush::Entity *>(entity));
}

void Hush__Scene__QueueEntityDestroy(Hush__Scene *self, Hush__Entity *entity,
									 Hush__EntityOnDestroyCallback_t destructor)
{
	auto selfClass = reinterpret_cast<Hush::Scene *>(self);
	selfClass->QueueEntityDestroy(*reinterpret_cast<Hush::Entity *>(entity),
								  reinterpret_cast<void (*)(unsigned long long, void *)>(destructor));
}

Hush__Entity Hush__Scene__EntityFromIdUnchecked(Hush__Scene *self, Hush__Entity_EntityId id)
{
	auto selfClass = reinterpret_cast<Hush::Scene *>(self);
	auto result______ = selfClass->EntityFromIdUnchecked(id);
	return *reinterpret_cast<Hush__Entity *>(&result______);
}

Hush__Entity_EntityId Hush__Scene__RegisterComponentRaw(Hush__Scene *self,
														const Hush__ComponentTraits__ComponentInfo *desc)
{
	auto selfClass = reinterpret_cast<Hush::Scene *>(self);
	auto result______ =
		selfClass->RegisterComponentRaw(*reinterpret_cast<const Hush::ComponentTraits::ComponentInfo *>(desc));
	return *reinterpret_cast<Hush__Entity_EntityId *>(&result______);
}

void Hush__Scene__MarkComponentToggleableRaw(Hush__Scene *self, Hush__Entity_EntityId id)
{
	auto selfClass = reinterpret_cast<Hush::Scene *>(self);
	selfClass->MarkComponentToggleableRaw(id);
}

Hush__Entity_EntityId Hush__Scene__Lookup(Hush__Scene *self, const char *tagData, const size_t tagSize)
{
	auto selfClass = reinterpret_cast<Hush::Scene *>(self);
	auto tagData__ = std::basic_string_view<char>(reinterpret_cast<const char *>(tagData), tagSize);
	auto result______ = selfClass->Lookup(tagData__);
	return *reinterpret_cast<Hush__Entity_EntityId *>(&result______);
}

Hush__RawQuery Hush__Scene__CreateRawQuery(Hush__Scene *self, Hush__Entity_EntityId *componentsData,
										   const size_t componentsSize, Hush__RawQuery__ECacheMode cacheMode)
{
	auto selfClass = reinterpret_cast<Hush::Scene *>(self);
	auto componentsData__ =
		std::span<unsigned long long>(reinterpret_cast<unsigned long long *>(componentsData), componentsSize);
	auto result______ = selfClass->CreateRawQuery(componentsData__, static_cast<Hush::RawQuery::ECacheMode>(cacheMode));
	std::aligned_storage_t<sizeof(Hush__RawQuery)> resultStorage_____;
	auto *resultPtr = reinterpret_cast<decltype(result______) *>(&resultStorage_____);
	new (resultPtr) decltype(result______)(std::move(result______));
	return *reinterpret_cast<Hush__RawQuery *>(resultPtr);
}

Hush__Scene *Hush__HushEngine__GetScene(Hush__HushEngine *self)
{
	auto selfClass = reinterpret_cast<Hush::HushEngine *>(self);
	auto result______ = selfClass->GetScene();
	return reinterpret_cast<Hush__Scene *>(result______);
}

Hush__HushEngine__EError Hush__HushEngine__LoadScene(Hush__HushEngine *self, Hush__Scene *scene)
{
	auto selfClass = reinterpret_cast<Hush::HushEngine *>(self);
	auto result______ = selfClass->LoadScene(reinterpret_cast<Hush::Scene *>(scene));
	return static_cast<Hush__HushEngine__EError>(result______);
}

bool Hush__Modules__SetForeignSystemRuntimeOps(Hush__HushEngine *engine, Hush__ModuleHandle module, const void *ops)
{
	auto result______ = Hush::Modules::SetForeignSystemRuntimeOps(reinterpret_cast<Hush::HushEngine *>(engine), module,
																  reinterpret_cast<const void *>(ops));
	return *reinterpret_cast<bool *>(&result______);
}

bool Hush__Modules__RegisterForeignSystem(Hush__HushEngine *engine, Hush__ModuleHandle module, const void *descriptor)
{
	auto result______ = Hush::Modules::RegisterForeignSystem(reinterpret_cast<Hush::HushEngine *>(engine), module,
															 reinterpret_cast<const void *>(descriptor));
	return *reinterpret_cast<bool *>(&result______);
}

bool Hush__Modules__AddSystemToScene(Hush__HushEngine *engine, Hush__Scene *scene, Hush__ModuleHandle module,
									 uint64_t typeId)
{
	auto result______ = Hush::Modules::AddSystemToScene(reinterpret_cast<Hush::HushEngine *>(engine),
														reinterpret_cast<Hush::Scene *>(scene), module, typeId);
	return *reinterpret_cast<bool *>(&result______);
}

bool Hush__Modules__RegisterComponentFromReflectedType(Hush__HushEngine *engine, Hush__Scene *scene, uint64_t typeId)
{
	auto result______ = Hush::Modules::RegisterComponentFromReflectedType(
		reinterpret_cast<Hush::HushEngine *>(engine), reinterpret_cast<Hush::Scene *>(scene), typeId);
	return *reinterpret_cast<bool *>(&result______);
}

bool Hush__Modules__HasReflectedType(Hush__HushEngine *engine, uint64_t typeId)
{
	auto result______ = Hush::Modules::HasReflectedType(reinterpret_cast<Hush::HushEngine *>(engine), typeId);
	return *reinterpret_cast<bool *>(&result______);
}

bool Hush__Modules__GetReflectedTypeOwner(Hush__HushEngine *engine, uint64_t typeId, Hush__ModuleHandle *owner)
{
	auto result______ = Hush::Modules::GetReflectedTypeOwner(reinterpret_cast<Hush::HushEngine *>(engine), typeId,
															 reinterpret_cast<unsigned long long *>(owner));
	return *reinterpret_cast<bool *>(&result______);
}

uint32_t Hush__Modules__GetReflectedTypeNameLength(Hush__HushEngine *engine, uint64_t typeId)
{
	auto result______ = Hush::Modules::GetReflectedTypeNameLength(reinterpret_cast<Hush::HushEngine *>(engine), typeId);
	return *reinterpret_cast<uint32_t *>(&result______);
}

bool Hush__Modules__CopyReflectedTypeName(Hush__HushEngine *engine, uint64_t typeId, char *destination,
										  uint32_t destinationSize)
{
	auto result______ = Hush::Modules::CopyReflectedTypeName(reinterpret_cast<Hush::HushEngine *>(engine), typeId,
															 reinterpret_cast<char *>(destination), destinationSize);
	return *reinterpret_cast<bool *>(&result______);
}

bool Hush__Modules__HasReflectedTypeMetadata(Hush__HushEngine *engine, uint64_t typeId, const char *keyData,
											 size_t keySize)
{
	auto result______ = Hush::Modules::HasReflectedTypeMetadata(reinterpret_cast<Hush::HushEngine *>(engine), typeId,
																reinterpret_cast<const char *>(keyData), keySize);
	return *reinterpret_cast<bool *>(&result______);
}

uint32_t Hush__Modules__GetReflectedTypeMetadataValueLength(Hush__HushEngine *engine, uint64_t typeId,
															const char *keyData, size_t keySize)
{
	auto result______ = Hush::Modules::GetReflectedTypeMetadataValueLength(
		reinterpret_cast<Hush::HushEngine *>(engine), typeId, reinterpret_cast<const char *>(keyData), keySize);
	return *reinterpret_cast<uint32_t *>(&result______);
}

bool Hush__Modules__CopyReflectedTypeMetadataValue(Hush__HushEngine *engine, uint64_t typeId, const char *keyData,
												   size_t keySize, char *destination, uint32_t destinationSize)
{
	auto result______ = Hush::Modules::CopyReflectedTypeMetadataValue(
		reinterpret_cast<Hush::HushEngine *>(engine), typeId, reinterpret_cast<const char *>(keyData), keySize,
		reinterpret_cast<char *>(destination), destinationSize);
	return *reinterpret_cast<bool *>(&result______);
}

void Hush__Transform__SetPosition(Hush__Transform *self, Vector3 position)
{
	auto selfClass = reinterpret_cast<Hush::Transform *>(self);
	selfClass->SetPosition(reinterpret_cast<const glm::vec3 &>(position));
}

Vector3 *Hush__Transform__GetPosition(Hush__Transform *self)
{
	auto selfClass = reinterpret_cast<Hush::Transform *>(self);
	auto result______ = selfClass->GetPosition();
	return reinterpret_cast<Vector3 *>(result______);
}

Vector3 Hush__Transform__GetPositionValue(Hush__Transform *self)
{
	auto selfClass = reinterpret_cast<Hush::Transform *>(self);
	auto result______ = selfClass->GetPositionValue();
	return *reinterpret_cast<Vector3 *>(&result______);
}

void Hush__Transform__SetScale(Hush__Transform *self, Vector3 scale)
{
	auto selfClass = reinterpret_cast<Hush::Transform *>(self);
	selfClass->SetScale(reinterpret_cast<const glm::vec3 &>(scale));
}

Vector3 Hush__Transform__GetScale(Hush__Transform *self)
{
	auto selfClass = reinterpret_cast<Hush::Transform *>(self);
	auto result______ = selfClass->GetScale();
	return *reinterpret_cast<Vector3 *>(&result______);
}

void Hush__Transform__SetRotationQuat(Hush__Transform *self, const Quat *rotationQuat)
{
	auto selfClass = reinterpret_cast<Hush::Transform *>(self);
	selfClass->SetRotationQuat(*reinterpret_cast<const glm::qua<float> *>(rotationQuat));
}

Quat Hush__Transform__GetRotationQuat(Hush__Transform *self)
{
	auto selfClass = reinterpret_cast<Hush::Transform *>(self);
	auto result______ = selfClass->GetRotationQuat();
	return *reinterpret_cast<Quat *>(&result______);
}

void Hush__Transform__SetEulerAngles(Hush__Transform *self, const Vector3 *euler)
{
	auto selfClass = reinterpret_cast<Hush::Transform *>(self);
	selfClass->SetEulerAngles(*reinterpret_cast<const glm::vec<3, float> *>(euler));
}

Vector3 Hush__Transform__GetEulerAngles(Hush__Transform *self)
{
	auto selfClass = reinterpret_cast<Hush::Transform *>(self);
	auto result______ = selfClass->GetEulerAngles();
	return *reinterpret_cast<Vector3 *>(&result______);
}

Vector3 Hush__Transform__Forward(Hush__Transform *self)
{
	auto selfClass = reinterpret_cast<Hush::Transform *>(self);
	auto result______ = selfClass->Forward();
	return *reinterpret_cast<Vector3 *>(&result______);
}

Vector3 Hush__Transform__Up(Hush__Transform *self)
{
	auto selfClass = reinterpret_cast<Hush::Transform *>(self);
	auto result______ = selfClass->Up();
	return *reinterpret_cast<Vector3 *>(&result______);
}

Vector3 Hush__Transform__Right(Hush__Transform *self)
{
	auto selfClass = reinterpret_cast<Hush::Transform *>(self);
	auto result______ = selfClass->Right();
	return *reinterpret_cast<Vector3 *>(&result______);
}

Matrix4 Hush__Transform__GetTransformationMatrix(Hush__Transform *self)
{
	auto selfClass = reinterpret_cast<Hush::Transform *>(self);
	auto result______ = selfClass->GetTransformationMatrix();
	return *reinterpret_cast<Matrix4 *>(&result______);
}

void Hush__Transform__GetTransformationMatrixUnsafe(Hush__Transform *self, float *outMatrix, size_t count)
{
	auto selfClass = reinterpret_cast<Hush::Transform *>(self);
	selfClass->GetTransformationMatrixUnsafe(reinterpret_cast<float *>(outMatrix), count);
}

Matrix4 Hush__Transform__XForm(Hush__Transform *self, const Hush__Transform *other)
{
	auto selfClass = reinterpret_cast<Hush::Transform *>(self);
	auto result______ = selfClass->XForm(*reinterpret_cast<const Hush::Transform *>(other));
	return *reinterpret_cast<Matrix4 *>(&result______);
}

Matrix4 Hush__Transform__InvXForm(Hush__Transform *self, const Hush__Transform *other)
{
	auto selfClass = reinterpret_cast<Hush::Transform *>(self);
	auto result______ = selfClass->InvXForm(*reinterpret_cast<const Hush::Transform *>(other));
	return *reinterpret_cast<Matrix4 *>(&result______);
}

bool Hush__InputManager__IsKeyDown(Hush__EKeyCode key)
{
	auto result______ = Hush::InputManager::IsKeyDown(static_cast<Hush::EKeyCode>(key));
	return *reinterpret_cast<bool *>(&result______);
}

bool Hush__InputManager__IsKeyDownThisFrame(Hush__EKeyCode key)
{
	auto result______ = Hush::InputManager::IsKeyDownThisFrame(static_cast<Hush::EKeyCode>(key));
	return *reinterpret_cast<bool *>(&result______);
}

bool Hush__InputManager__IsKeyUp(Hush__EKeyCode key)
{
	auto result______ = Hush::InputManager::IsKeyUp(static_cast<Hush::EKeyCode>(key));
	return *reinterpret_cast<bool *>(&result______);
}

bool Hush__InputManager__IsKeyHeld(Hush__EKeyCode key)
{
	auto result______ = Hush::InputManager::IsKeyHeld(static_cast<Hush::EKeyCode>(key));
	return *reinterpret_cast<bool *>(&result______);
}

bool Hush__InputManager__GetMouseButtonPressed(Hush__EMouseButton button)
{
	auto result______ = Hush::InputManager::GetMouseButtonPressed(static_cast<Hush::EMouseButton>(button));
	return *reinterpret_cast<bool *>(&result______);
}

bool Hush__InputManager__FetchCharThisFrame(char *outChar)
{
	auto result______ = Hush::InputManager::FetchCharThisFrame(reinterpret_cast<char *>(outChar));
	return *reinterpret_cast<bool *>(&result______);
}

Vector2 Hush__InputManager__GetMousePosition(void)
{
	auto result______ = Hush::InputManager::GetMousePosition();
	return *reinterpret_cast<Vector2 *>(&result______);
}

Vector2 Hush__InputManager__GetMouseAcceleration(void)
{
	auto result______ = Hush::InputManager::GetMouseAcceleration();
	return *reinterpret_cast<Vector2 *>(&result______);
}

void Hush__InputManager__SetCursorLock(Hush__ECursorLockMode lockMode)
{
	Hush::InputManager::SetCursorLock(static_cast<Hush::ECursorLockMode>(lockMode));
}

Vector3 Hush__Camera__ScreenToWorldPos(Hush__Camera *self, Matrix4 viewMatrix, Vector2 mousePos, Vector3 *outDirection)
{
	auto selfClass = reinterpret_cast<Hush::Camera *>(self);
	auto result______ = selfClass->ScreenToWorldPos(reinterpret_cast<const glm::mat4 &>(viewMatrix),
													reinterpret_cast<const glm::vec2 &>(mousePos),
													reinterpret_cast<glm::vec<3, float> *>(outDirection));
	return *reinterpret_cast<Vector3 *>(&result______);
}

Vector3 Hush__Camera__ScreenToWorldPosUnsafe(Hush__Camera *self, float *viewMatrix, Vector2 mousePos,
											 Vector3 *outDirection)
{
	auto selfClass = reinterpret_cast<Hush::Camera *>(self);
	auto result______ = selfClass->ScreenToWorldPosUnsafe(reinterpret_cast<float *>(viewMatrix),
														  reinterpret_cast<const glm::vec2 &>(mousePos),
														  reinterpret_cast<glm::vec<3, float> *>(outDirection));
	return *reinterpret_cast<Vector3 *>(&result______);
}

Vector3 Hush__Camera__ProjectPlanePosition(Hush__Camera *self, Vector3 origin, Vector3 direction, float height)
{
	auto selfClass = reinterpret_cast<Hush::Camera *>(self);
	auto result______ = selfClass->ProjectPlanePosition(reinterpret_cast<const glm::vec3 &>(origin),
														reinterpret_cast<const glm::vec3 &>(direction), height);
	return *reinterpret_cast<Vector3 *>(&result______);
}

float Hush__Camera__GetFarPlane(Hush__Camera *self)
{
	auto selfClass = reinterpret_cast<Hush::Camera *>(self);
	auto result______ = selfClass->GetFarPlane();
	return *reinterpret_cast<float *>(&result______);
}

Hush__Graphics__Material3D__EError Hush__Graphics__Material3D__SetPropertyRaw(Hush__Graphics__Material3D *self,
																			  const char *nameData,
																			  const size_t nameSize, uint8_t *valueData,
																			  const size_t valueSize)
{
	auto selfClass = reinterpret_cast<Hush::Graphics::Material3D *>(self);
	auto nameData__ = std::basic_string_view<char>(reinterpret_cast<const char *>(nameData), nameSize);
	auto valueData__ = std::span<const unsigned char>(reinterpret_cast<const unsigned char *>(valueData), valueSize);
	auto result______ = selfClass->SetPropertyRaw(nameData__, valueData__);
	return static_cast<Hush__Graphics__Material3D__EError>(result______);
}

Hush__Graphics__Material3D__EError Hush__Graphics__Material3D__GetPropertyRaw(Hush__Graphics__Material3D *self,
																			  const char *nameData,
																			  const size_t nameSize, uint8_t *outValue,
																			  size_t size)
{
	auto selfClass = reinterpret_cast<Hush::Graphics::Material3D *>(self);
	auto nameData__ = std::basic_string_view<char>(reinterpret_cast<const char *>(nameData), nameSize);
	auto result______ = selfClass->GetPropertyRaw(nameData__, reinterpret_cast<unsigned char *>(outValue), size);
	return static_cast<Hush__Graphics__Material3D__EError>(result______);
}

size_t Hush__MeshReference__GetMaterialCount(Hush__MeshReference *self)
{
	auto selfClass = reinterpret_cast<Hush::MeshReference *>(self);
	auto result______ = selfClass->GetMaterialCount();
	return *reinterpret_cast<size_t *>(&result______);
}

Hush__Graphics__Material3D *Hush__MeshReference__GetMaterialAt(Hush__MeshReference *self, size_t index)
{
	auto selfClass = reinterpret_cast<Hush::MeshReference *>(self);
	auto result______ = selfClass->GetMaterialAt(index);
	return reinterpret_cast<Hush__Graphics__Material3D *>(result______);
}

void Hush__MeshReference__CalculateBounds(Hush__MeshReference *self, Vector3 *outCenter, Vector3 *outSize)
{
	auto selfClass = reinterpret_cast<Hush::MeshReference *>(self);
	selfClass->CalculateBounds(reinterpret_cast<glm::vec<3, float> *>(outCenter),
							   reinterpret_cast<glm::vec<3, float> *>(outSize));
}

#ifdef HUSH_STATIC_BINDING
HushFuncPtrTable HUSH_FUNCPTR_TABLE = {
	Hush__Reflection__TypeInfo__SetName,
	Hush__Reflection__TypeInfo__SetSize,
	Hush__Reflection__TypeInfo__SetAlignment,
	Hush__Reflection__ReflectionDB__RegisterForeignClass,
	Hush__Reflection__ReflectionDB__AddFieldToForeignClass,
	Hush__Entity__RegisterComponentRaw,
	Hush__Entity__AddComponentRaw,
	Hush__Entity__AddId,
	Hush__Entity__GetComponentRaw,
	Hush__Entity__GetComponentConstRaw,
	Hush__Entity__HasComponentRaw,
	Hush__Entity__EmplaceComponentRaw,
	Hush__Entity__RemoveComponentRaw,
	Hush__Entity__SetComponentActiveRaw,
	Hush__Entity__AddChild,
	Hush__Entity__GetChildCount,
	Hush__Entity__AddRelationship,
	Hush__Entity__GetId,
	Hush__Entity__SetName,
	Hush__Entity__SetKey,
	Hush__Entity__GetKey,
	Hush__Entity__QueryName,
	Hush__Entity__IsAlive,
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
	Hush__impl__QueryBuilderImpl__Without,
	Hush__impl__QueryBuilderImpl__WithOptional,
	Hush__impl__QueryBuilderImpl__IncludeTemplates,
	Hush__impl__QueryBuilderImpl__TemplatesOnly,
	Hush__impl__QueryBuilderImpl__InitQuery,
	Hush__OpaqueQueryDescriptor__data,
	Hush__Scene__RemoveSystem,
	Hush__Scene__CreateEntity,
	Hush__Scene__CreateEntityWithName,
	Hush__Scene__CreateEntityWithKey,
	Hush__Scene__AddComponentObserverRaw,
	Hush__Scene__DestroyEntity,
	Hush__Scene__QueueEntityDestroy,
	Hush__Scene__EntityFromIdUnchecked,
	Hush__Scene__RegisterComponentRaw,
	Hush__Scene__MarkComponentToggleableRaw,
	Hush__Scene__Lookup,
	Hush__Scene__CreateRawQuery,
	Hush__HushEngine__GetScene,
	Hush__HushEngine__LoadScene,
	Hush__Modules__SetForeignSystemRuntimeOps,
	Hush__Modules__RegisterForeignSystem,
	Hush__Modules__AddSystemToScene,
	Hush__Modules__RegisterComponentFromReflectedType,
	Hush__Modules__HasReflectedType,
	Hush__Modules__GetReflectedTypeOwner,
	Hush__Modules__GetReflectedTypeNameLength,
	Hush__Modules__CopyReflectedTypeName,
	Hush__Modules__HasReflectedTypeMetadata,
	Hush__Modules__GetReflectedTypeMetadataValueLength,
	Hush__Modules__CopyReflectedTypeMetadataValue,
	Hush__Transform__SetPosition,
	Hush__Transform__GetPosition,
	Hush__Transform__GetPositionValue,
	Hush__Transform__SetScale,
	Hush__Transform__GetScale,
	Hush__Transform__SetRotationQuat,
	Hush__Transform__GetRotationQuat,
	Hush__Transform__SetEulerAngles,
	Hush__Transform__GetEulerAngles,
	Hush__Transform__Forward,
	Hush__Transform__Up,
	Hush__Transform__Right,
	Hush__Transform__GetTransformationMatrix,
	Hush__Transform__GetTransformationMatrixUnsafe,
	Hush__Transform__XForm,
	Hush__Transform__InvXForm,
	Hush__InputManager__IsKeyDown,
	Hush__InputManager__IsKeyDownThisFrame,
	Hush__InputManager__IsKeyUp,
	Hush__InputManager__IsKeyHeld,
	Hush__InputManager__GetMouseButtonPressed,
	Hush__InputManager__FetchCharThisFrame,
	Hush__InputManager__GetMousePosition,
	Hush__InputManager__GetMouseAcceleration,
	Hush__InputManager__SetCursorLock,
	Hush__Camera__ScreenToWorldPos,
	Hush__Camera__ScreenToWorldPosUnsafe,
	Hush__Camera__ProjectPlanePosition,
	Hush__Camera__GetFarPlane,
	Hush__Graphics__Material3D__SetPropertyRaw,
	Hush__Graphics__Material3D__GetPropertyRaw,
	Hush__MeshReference__GetMaterialCount,
	Hush__MeshReference__GetMaterialAt,
	Hush__MeshReference__CalculateBounds,
};
#endif
// NOLINTEND
