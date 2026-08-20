
#include "HierarchyPanel.hpp"
#include <cstdint>
#include <functional>
#include <imgui/imgui.h>
#include <Assertions.hpp>
#include <optional>
#include <string_view>
#include "Components/LocalTransform.hpp"
#include "Components/Serializable.hpp"
#include "Components/WorldTransform.hpp"
#include "Entity.hpp"
#include "InspectorPanel.hpp"
#include "UI.hpp"
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

	this->m_activeScene = activeScene;
	this->m_inspectableEntitiesQuery = this->m_activeScene->CreateQuery<WorldTransform, LocalTransform, Entity::Name>();
}

void Hush::HierarchyPanel::OnRender([[maybe_unused]] float deltaTime)
{
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

void Hush::HierarchyPanel::GenerateEntitySelectableTree(const Entity &entity, const Entity::Name &name,
														InspectorPanel *inspector)
{
	ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_DrawLinesToNodes;
	if (entity.GetChildCount() < 1)
	{
		flags |= ImGuiTreeNodeFlags_Leaf;
	}

	bool isNodeOpen = ImGui::TreeNodeEx(name.GetName().data(), flags);
	if (ImGui::IsItemClicked())
	{
		inspector->SetInspectTarget(entity.GetId());
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
}
