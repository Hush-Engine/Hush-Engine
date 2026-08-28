#include "InspectorPanel.hpp"
#include "Assertions.hpp"
#include "BitwiseUtils.hpp"
#include "Components/LocalTransform.hpp"
#include "Components/Material3D.hpp"
#include "Components/MeshReference.hpp"
#include "Components/Transform.hpp"
#include "Components/WorldTransform.hpp"
#include "HushEngine.hpp"
#include "Logger.hpp"
#include "RHI/ShaderCompiler.hpp"
#include "Ref.hpp"
#include "ScriptingHost.hpp"
#include "Shared/Camera.hpp"
#include "components/EditorInfo.hpp"
#include "imgui/imgui.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/trigonometric.hpp>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
#include "UI.hpp"
#include "ScenePanel.hpp"
#include "Shared/DirectionalLight.hpp"
#include <memory>

constexpr float NESTED_INDENT_SIZE = 10.0F;

// Some temp auxiliar functions
std::string ConcatCStr(const std::string_view &base, const std::string_view &other)
{
	return std::string(base) + other.data();
}

void Hush::Serialize(Camera *cam)
{
	ImGui::CollapsingHeader("Camera", ImGuiTreeNodeFlags_DefaultOpen);
	float fov = cam->GetFOV();
	if (ImGui::SliderFloat("Field Of View", &fov, 0, 100))
	{
		cam->SetFOV(fov);
	}
}

void Hush::Serialize(DirectionalLight *component)
{
	ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen);
	glm::vec4 &rgba = component->color.GetRGBA32F();
	auto *rgbRegion = reinterpret_cast<float *>(&rgba);
	ImGui::ColorEdit3("Light Color", rgbRegion);
	ImGui::InputFloat("Intensity", &component->intensity);
}

void Hush::Serialize(Hush::Graphics::Material3D *component, size_t idx)
{
	std::string_view name = component->GetName();
	ImGui::Text("%s#%zu", name.data(), idx);

	auto drawPropertyFlags = [](const char *label, Graphics::EBindingDataTypeFlags *typeFlags) {
		if (UI::FlagsBegin(label))
		{
			UI::FlagItem("As Color", Graphics::EBindingDataTypeFlags::AsColor, typeFlags);
			UI::FlagItem("Hide", Graphics::EBindingDataTypeFlags::IsPrivate, typeFlags);
			UI::FlagsEnd();
		}
		ImGui::SameLine();
	};

	component->OnEachPropertyMut([&drawPropertyFlags](std::string_view propName,
													  Graphics::MaterialPropertyInfo *infoRef,
													  std::span<std::byte> uniformRange) {
		// NYI: We should still show something about this property, but have it collapsed and grayed out or something
		if (Bitwise::HasCompositeFlag(infoRef->typeFlags, Graphics::EBindingDataTypeFlags::IsPrivate))
		{
			return false;
		}
		ImGui::PushID(propName.data());
		Graphics::EBindingDataTypeFlags flags = infoRef->typeFlags;

		// This indicates whether the material's CPU-side buffer was modified, NOT the property's flags or any other
		// metadata
		bool wasModified = false;
		bool asColor = Bitwise::HasCompositeFlag(flags, Graphics::EBindingDataTypeFlags::AsColor);

		if (Bitwise::HasCompositeFlag(flags, Graphics::EBindingDataTypeFlags::Vec3))
		{
			drawPropertyFlags(propName.data(), &infoRef->typeFlags);
			HUSH_ASSERT(uniformRange.size_bytes() == sizeof(glm::vec3), "Property type does not map to its size!");
			if (asColor)
			{
				wasModified = ImGui::ColorEdit3("##color", reinterpret_cast<float *>(uniformRange.data()));
			}
			else
			{
				wasModified = UI::Vec3Edit("##vec3", reinterpret_cast<float *>(uniformRange.data()));
			}
		}
		else if (Bitwise::HasCompositeFlag(flags, Graphics::EBindingDataTypeFlags::Vec4))
		{
			drawPropertyFlags(propName.data(), &infoRef->typeFlags);
			HUSH_ASSERT(uniformRange.size_bytes() == sizeof(glm::vec4), "Property type does not map to its size!");
			if (asColor)
			{
				wasModified = ImGui::ColorEdit4("##color", reinterpret_cast<float *>(uniformRange.data()));
			}
			else
			{
				wasModified = UI::Vec4Edit("##vec4", reinterpret_cast<float *>(uniformRange.data()));
			}
		}

		ImGui::PopID();
		return wasModified;
	});
}

void Hush::Serialize(MeshReference *component, const char *entityName)
{
	if (!ImGui::CollapsingHeader((std::string("Mesh Component##") + entityName).c_str(),
								 ImGuiTreeNodeFlags_DefaultOpen))
	{
		return;
	}
	ImGui::Text("Name: %s", component->GetMesh()->GetName().c_str());
	// TODO: Maybe write this as a table
	ImGui::Indent(NESTED_INDENT_SIZE);

	const std::vector<Ref<Graphics::Material3D>> &materials = component->GetMaterials();

	for (size_t i = 0; i < materials.size(); i++)
	{
		const Ref<Graphics::Material3D> &currMat = materials[i];
		Serialize(const_cast<Graphics::Material3D *>(currMat.Get()), i);
	}

	ImGui::Unindent(NESTED_INDENT_SIZE);
}

void Hush::Serialize(Transform *component)
{
	if (!ImGui::CollapsingHeader("Transform", ImGuiTreeNodeFlags_DefaultOpen))
	{
		return;
	}

	glm::vec3 *pos = component->GetPosition();
	glm::vec3 scale = component->GetScale();
	glm::vec3 rot = glm::degrees(component->GetEulerAngles());
	UI::Vec3Edit("Position", reinterpret_cast<float *>(pos));

	if (UI::Vec3Edit("Rotation", reinterpret_cast<float *>(&rot)))
	{
		component->SetRotationQuat(glm::quat(glm::radians(rot)));
	}

	if (UI::Vec3Edit("Scale", reinterpret_cast<float *>(&scale)))
	{
		component->SetScale(scale);
	}
}

void Hush::InspectorPanel::OnRender([[maybe_unused]] float deltaTime)
{
	ImGui::Begin("Inspector");
	if (this->m_inspectTarget.has_value())
	{
		this->RenderProperties();
	}
	ImGui::End();
}

void Hush::InspectorPanel::Init(Scene *activeScene) noexcept
{
	this->m_activeScene = activeScene;

	activeScene->CreateQuery<EditorInfo, ScriptingHost>().Each([this]([[maybe_unused]]
																	  Entity &entity,
																	  EditorInfo &infoRef,
																	  ScriptingHost &scriptingHostRef) {
		this->m_editorInfo = &infoRef;
		this->m_scriptingHost = &scriptingHostRef;
	});
}

void Hush::InspectorPanel::SetInspectTarget(Entity::EntityId entity)
{
	this->m_inspectTarget = this->m_activeScene->EntityFromId(entity);
	UI::Get().GetPanel<ScenePanel>().SetGizmoTarget(entity);
}

const std::optional<Hush::Entity> &Hush::InspectorPanel::GetInspectTarget() const
{
	return this->m_inspectTarget;
}

std::optional<Hush::Entity> &Hush::InspectorPanel::GetInspectTarget()
{
	return this->m_inspectTarget;
}

void UserComponentRenderProps(Hush::Entity::EntityId id, uint8_t *instance,
							  const Hush::ScriptingComponentSerializationInfo &compSerialInfo)
{
	using namespace Hush;
	// Iterate over the properties of the type
	std::string title = std::format("{}#{}", &(compSerialInfo.name[0]), id);
	ImGui::CollapsingHeader(title.c_str(), ImGuiTreeNodeFlags_DefaultOpen);

	for (uint32_t i = 0; i < compSerialInfo.propertyCount; i++)
	{
		const ScriptingComponentPropertyInfo* prop = &(compSerialInfo.properties[i]);
		// Now we render depending on the type
		uint8_t* ptrToSerialize = instance + prop->offset;
		std::string_view propertyName = &(prop->name[0]);

		switch (prop->type) {
		case Hush::EComponentPropertyType::I8:
			ImGui::InputScalar(propertyName.data(), ImGuiDataType_S8, ptrToSerialize);
			break;
		case Hush::EComponentPropertyType::U8:
			ImGui::InputScalar(propertyName.data(), ImGuiDataType_U8, ptrToSerialize);
			break;
		case Hush::EComponentPropertyType::I16:
			ImGui::InputScalar(propertyName.data(), ImGuiDataType_S16, ptrToSerialize);
			break;
		case Hush::EComponentPropertyType::U16:
			ImGui::InputScalar(propertyName.data(), ImGuiDataType_U16, ptrToSerialize);
			break;
		case Hush::EComponentPropertyType::I32:
			ImGui::InputScalar(propertyName.data(), ImGuiDataType_S32, ptrToSerialize);
			break;
		case Hush::EComponentPropertyType::U32:
			ImGui::InputScalar(propertyName.data(), ImGuiDataType_U32, ptrToSerialize);
			break;
		case Hush::EComponentPropertyType::I64:
			ImGui::InputScalar(propertyName.data(), ImGuiDataType_S64, ptrToSerialize);
			break;
		case Hush::EComponentPropertyType::U64:
			ImGui::InputScalar(propertyName.data(), ImGuiDataType_U64, ptrToSerialize);
			break;
		case Hush::EComponentPropertyType::F32:
			ImGui::InputScalar(propertyName.data(), ImGuiDataType_Float, ptrToSerialize);
			break;
		case Hush::EComponentPropertyType::F64:
			ImGui::InputScalar(propertyName.data(), ImGuiDataType_Double, ptrToSerialize);
			break;
		case Hush::EComponentPropertyType::Bool:
			ImGui::Checkbox(propertyName.data(), reinterpret_cast<bool*>(ptrToSerialize));
			break;
		default:
			// LogError("Type of property is not implemented for inspector serialization");
			break;
		}
	}
}

// TODO: This probably should be a query on components that hold a function pointer on how to get serialized
// Also, every entity in the editor should probably have a list of how they ordered their components ???
void Hush::InspectorPanel::RenderProperties()
{
	Entity::Name *entityName = this->m_inspectTarget->GetComponent<Entity::Name>();
	HUSH_ASSERT(entityName != nullptr, "Inspectable entities MUST have a name component!");
	ImGui::SeparatorText(entityName->GetName().data());
	LocalTransform *transform = this->m_inspectTarget->GetComponent<LocalTransform>();
	HUSH_ASSERT(transform != nullptr, "Trying to render an entity without a Transform component!");
	Serialize(transform);
	// Get all the other entity's components
	// TODO: handle this with reflection
	DirectionalLight *dirLightComponent = this->m_inspectTarget->GetComponent<DirectionalLight>();
	if (dirLightComponent != nullptr)
	{
		Serialize(dirLightComponent);
	}

	MeshReference *meshComponent = this->m_inspectTarget->GetComponent<MeshReference>();
	if (meshComponent != nullptr)
	{
		Serialize(meshComponent, entityName->GetName().data());
	}

	Camera *camComponent = this->m_inspectTarget->GetComponent<Camera>();
	if (camComponent != nullptr)
	{
		Serialize(camComponent);
	}

	// Now, for any user defined components, we use the functions inside of their archetype
	// HACK: Super bad awful approach
	const std::vector<ScriptingComponentSerializationInfo> &serializableComps =
		this->m_scriptingHost->GetAvailableComponentSerializationData();

	for (const auto &compSerialInfo : serializableComps)
	{
		std::string_view compName = &(compSerialInfo.name[0]);

		// PERF: THIS IS VERY BAD, WE ONLY DID THIS FOR THE JAM, BUT IT'S SUPER BAD
		Entity::EntityId comp = this->m_activeScene->Lookup(compName);
		if (!this->m_inspectTarget.value().HasComponentRaw(comp))
		{
			continue;
		}

		void *compMem = this->m_inspectTarget.value().GetComponentRaw(comp);
		UserComponentRenderProps(comp, reinterpret_cast<uint8_t *>(compMem), compSerialInfo);
	}
}
