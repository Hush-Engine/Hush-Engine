
#include "HierarchyPanel.hpp"
#include <cstdint>
#include <functional>
#include <imgui/imgui.h>
#include <Assertions.hpp>
#include <memory_resource>
#include <optional>
#include <string_view>
#include "Components/GlobalKeys.hpp"
#include "Components/LocalTransform.hpp"
#include "Components/Serializable.hpp"
#include "Components/WorldTransform.hpp"
#include "Entity.hpp"
#include "InputManager.hpp"
#include "InspectorPanel.hpp"
#include "Logger.hpp"
#include "UI.hpp"
#include "components/EditorInfo.hpp"
#include "definitions/KeyCode.hpp"
#include "serialization/Formats/JsonSerializer.hpp"
#include "serialization/Serialization.hpp"

void Hush::HierarchyPanel::Init(Scene *activeScene) noexcept
{

	Entity::EntityId nameId = activeScene->RegisterComponent<Entity::Name>();
	Entity nameComp = activeScene->EntityFromIdUnchecked(nameId);
	{
		Serializable &ser = nameComp.AddComponent<Serializable>();
		ser.serialize = [](const uint8_t *instance, Serialization::JsonSerializer &ser, void *ctx) {
			(void)ctx;
			const auto *nameInstance = reinterpret_cast<const Entity::Name *>(instance);
			Serialization::ESerializationError err = ser.Serialize("name", nameInstance->GetName());
			if (err != Serialization::ESerializationError::None)
			{
				return Serializable::EError::ParseError;
			}
			return Serializable::EError::None;
		};
		ser.deserialize = [](uint8_t *instance, Serialization::JsonDeserializer &deser, void *ctx) {
			(void)ctx;
			auto *nameInstance = reinterpret_cast<Entity::Name *>(instance);

			std::string_view k;
			int64_t i = 0;
			// HACK: Skip these
			(void)deser.Next();
			(void)deser.ReadKey(k);
			(void)deser.ReadInt(i);
			(void)deser.ReadKey(k);
			(void)deser.ReadString(k);

			(void)deser.ReadKey(k);
			(void)deser.ReadString(k);

			nameInstance->SetName(k);
			(void)deser.Next();
			return Serializable::EError::None;
		};
	}

	Entity engineManager = activeScene->CreateEntityWithKey(ENGINE_MANAGER);
	this->m_editorInfo = engineManager.CreateComponentReference<EditorInfo>();
	this->m_activeScene = activeScene;
	this->m_inspectableEntitiesQuery = this->m_activeScene->CreateQuery<WorldTransform, LocalTransform, Entity::Name>();
}

void Hush::HierarchyPanel::OnRender([[maybe_unused]] float deltaTime)
{
	this->HandleInput();
	ImGuiViewport *mainViewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowViewport(mainViewport->ID);
	ImGui::Begin("Hierarchy");
	auto &inspectorPanel = UI::Get().GetPanel<InspectorPanel>();
	// Maybe draw them in separate systems?
	this->m_inspectableEntitiesQuery.Each([&inspectorPanel, this](Entity &entity, [[maybe_unused]] WorldTransform &_,
																  [[maybe_unused]]
																  LocalTransform &localxForm,
																  Entity::Name &name) {
		if (entity.GetParent().IsValid())
		{
			// We skip rendering the entity because the parent would have already rendered itj
			return;
		}
		// bool selected = inspectorPanel.GetInspectTarget().has_value() &&
		// 				inspectorPanel.GetInspectTarget()->GetId() == entity.GetId();
		auto narrowedId = static_cast<int32_t>(entity.GetId());
		ImGui::PushID(narrowedId);

		this->GenerateEntitySelectableTree(entity, name, &inspectorPanel);

		ImGui::PopID();
	});

	ImGui::End();
}

void Hush::HierarchyPanel::HandleInput() {
	if (InputManager::IsKeyDownThisFrame(EKeyCode::DEL)){
		// Nesting bc I don't want to fetch this comp every frame
		auto* editorInfo = this->m_editorInfo.GetData<EditorInfo>();
		SelectedItemInfo& selection = editorInfo->currentSelection;
		if (selection.type == ESelectedItemType::Entity && selection.value != Entity::INVALID_ENTITY_ID) {
			// Add to the deletion queue
			Entity::QueueDestroy(this->m_activeScene->EntityFromIdUnchecked(selection.value));
			// Should we set the editorInfo comp to a selection of none??
		}
	}
}

void Hush::HierarchyPanel::GenerateEntitySelectableTree(const Entity &entity, const Entity::Name &name,
														InspectorPanel *inspector)
{
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DrawLinesToNodes;
	if (entity.GetChildCount() < 1)
	{
		flags |= ImGuiTreeNodeFlags_Leaf;
	}
	// std::pmr::memory_resource* allocator = this->m_activeScene->GetFrameScopeMemoryResource();

	// PERF: Temp strings
	std::string_view key = entity.GetKey();
	std::string display {name.GetName()};
	if (!key.empty()) {
		display.append(" (");
		display.append(key);
		display.append(")");
	}
	ImGui::BeginGroup();
	bool isNodeOpen = ImGui::TreeNodeEx(display.c_str(), flags);
	if (ImGui::IsItemClicked())
	{
		inspector->SetInspectTarget(entity.GetId());
	}

	if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
		ImGui::Text("Move: %s", display.c_str());
		Entity::EntityId entityToMoveId = entity.GetId();
		ImGui::SetDragDropPayload("MOVED_ENTITY", &entityToMoveId, sizeof(Entity::EntityId), ImGuiCond_Always);

		ImGui::EndDragDropSource();
	}

	if (ImGui::BeginDragDropTarget()) {
		const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MOVED_ENTITY");
		if (payload != nullptr) {
			Entity::EntityId id = *(reinterpret_cast<Entity::EntityId*>(payload->Data));
			Entity toMove = this->m_activeScene->EntityFromIdUnchecked(id);

			toMove.SetParent(entity);

		}
		ImGui::EndDragDropTarget();
	}

	if (isNodeOpen)
	{
		entity.EachChild([this, inspector](Entity &currChild) {
			Entity::Name *childName = currChild.GetComponent<Entity::Name>();
			if (childName == nullptr)
			{
				// Skip bc it's not renderable
				return;
			}
			GenerateEntitySelectableTree(currChild, *childName, inspector);
		});
		ImGui::TreePop();
	}
	ImGui::EndGroup();
}
