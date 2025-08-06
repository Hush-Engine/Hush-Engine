#include "InspectorPanel.hpp"
#include "Assertions.hpp"
#include "Components/MeshReference.hpp"
#include "Components/WorldTransform.hpp"
#include "Entity.hpp"
#include "HushEngine.hpp"
#include "InputManager.hpp"
#include "Renderer.hpp"
#include "Shared/EditorCamera.hpp"
#include "Shared/IMaterial3D.hpp"
#include "Shared/Mesh.hpp"
#include "Vulkan/GltfMetallicRoughness.hpp"
#include "WindowManager.hpp"
#include "components/EditorInfo.hpp"
#include "definitions/KeyCode.hpp"
#include "imgui/imgui.h"
#include <glm/ext/quaternion_float.hpp>
#include <glm/ext/vector_float3.hpp>
#include <glm/trigonometric.hpp>
#include <optional>
#include <string>
#include <string_view>
#include <vector>
#include "UI.hpp"
#include "Shared/DirectionalLight.hpp"
#include <memory>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#include "imguizmo/ImGuizmo.h"

constexpr float NESTED_INDENT_SIZE = 10.0F;

// Some temp auxiliar functions
std::string ConcatCStr(const std::string_view &base, const std::string_view &other)
{
	return std::string(base) + other.data();
}

void Hush::Serialize(DirectionalLight *component)
{
	ImGui::CollapsingHeader("Directional Light", ImGuiTreeNodeFlags_DefaultOpen);
	glm::vec4 &rgba = component->color.GetRGBA32F();
	auto *rgbRegion = reinterpret_cast<float *>(&rgba);
	ImGui::ColorEdit3("Light Color", rgbRegion);
	ImGui::InputFloat("Intensity", &component->intensity);
}

void Hush::Serialize(IMaterial3D *component, const char *uniqueName)
{

	ImGui::Text("Material: %s", component->GetName().c_str());

	// ECullMode cullMode = component->GetCullMode();
	// Check which instance of the material is
	// TODO: Do this with the reflection API instead of dynamic casting
	auto *pbrMaterial = dynamic_cast<GLTFMetallicRoughness *>(component);
	if (pbrMaterial == nullptr)
	{
		return;
	}

	// Albedo color
	glm::vec4 &albedo = pbrMaterial->GetAlbedo();
	ImGui::ColorEdit4(ConcatCStr("Albedo##", component->GetName()).c_str(), reinterpret_cast<float *>(&albedo));

	glm::vec3 &emission = pbrMaterial->GetEmissionColor();
	ImGui::ColorEdit3(ConcatCStr("Emission##", component->GetName()).c_str(), reinterpret_cast<float *>(&emission));

	// TODO: Turn the float setters into references (try to reconcile this with CTRL + Z)

	float emissionFactor = pbrMaterial->EmissionFactor();
	ImGui::InputFloat(ConcatCStr("Emission Factor##", component->GetName()).c_str(), &emissionFactor);

	pbrMaterial->SetEmissionFactor(emissionFactor);

	float roughness = pbrMaterial->GetRoughnessFactor();

	ImGui::SliderFloat(ConcatCStr("Roughness Factor##", component->GetName()).c_str(), &roughness, 0.F, 1.0F);
	pbrMaterial->SetRoughnessFactor(roughness);

	float metallic = pbrMaterial->GetMetallicFactor();
	ImGui::SliderFloat(ConcatCStr("Metallic Factor##", component->GetName()).c_str(), &metallic, -1.0F, 1.0F);
	pbrMaterial->SetMetallicFactor(metallic);

	float alphaThreshold = pbrMaterial->GetAlphaThreshold();
	ImGui::SliderFloat(ConcatCStr("Alpha Threshold##", component->GetName()).c_str(), &alphaThreshold, 0.0F, 1.0F);
	pbrMaterial->SetAlphaThreshold(alphaThreshold);
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
	const std::unordered_set<GeoSurface, GeoSurface::GeoSurfaceHash> &surfaces = component->GetMesh()->GetSurfaces();
	// Iterate over the surfaces and  serialize their materials as submeshes
	size_t surfaceIndex = 0;
	for (const GeoSurface& surface : surfaces)
	{
		if (ImGui::CollapsingHeader((std::string("Surface") + std::to_string(surfaceIndex)).c_str(),
									ImGuiTreeNodeFlags_DefaultOpen))
		{
			Serialize(surface.material.get(), std::to_string(surfaceIndex).c_str());
		}
		surfaceIndex++;
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
	ImGui::Text("Position");
	ImGui::InputFloat3("##Position", reinterpret_cast<float *>(pos));

	ImGui::Text("Rotation");
	ImGui::InputFloat3("##Rotation", reinterpret_cast<float *>(&rot));

	ImGui::Text("Scale");
	ImGui::InputFloat3("##Scale", reinterpret_cast<float *>(&scale));
	if (scale != component->GetScale())
	{
		component->SetScale(scale);
	}
	if (rot != glm::degrees(component->GetEulerAngles()))
	{
		component->SetRotationQuat(glm::quat(glm::radians(rot)));
	}
}

void Hush::InspectorPanel::OnRender()
{
	ImGui::Begin("Inspector");
	if (this->m_inspectTarget.has_value())
	{
		this->RenderProperties();
		this->RenderGizmo();
	}
	ImGui::End();
}

void Hush::InspectorPanel::Init(Scene *activeScene) noexcept
{
	// ImGuizmo::SetGizmoSizeClipSpace(2.0f);
	this->m_activeScene = activeScene;

	activeScene->CreateQuery<EditorInfo>().Each([this](Entity& entity, EditorInfo& infoRef) {
		this->m_editorInfo = &infoRef;
    });
}

void Hush::InspectorPanel::SetInspectTarget(Entity::EntityId entity)
{
	this->m_inspectTarget = this->m_activeScene->EntityFromId(entity);
	this->m_targetName = this->m_inspectTarget.value().GetComponent<Entity::Name>();
}

const std::optional<Hush::Entity> &Hush::InspectorPanel::GetInspectTarget() const
{
	return this->m_inspectTarget;
}

std::optional<Hush::Entity> &Hush::InspectorPanel::GetInspectTarget()
{
	return this->m_inspectTarget;
}

void Hush::InspectorPanel::RenderProperties()
{
	ImGui::SeparatorText(this->m_targetName->name.data());
	WorldTransform *transform = this->m_inspectTarget->GetComponent<WorldTransform>();
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
		Serialize(meshComponent, this->m_targetName->name.data());
	}
}


void Hush::InspectorPanel::RenderGizmo() {
	if (this->m_editorInfo->currentState == EEditorState::None) {
		if (InputManager::IsKeyDownThisFrame(EKeyCode::R)) {
			this->m_currentGizmoOp = ImGuizmo::ROTATE;
		}
	
		if (InputManager::IsKeyDownThisFrame(EKeyCode::T)) {
			this->m_currentGizmoOp = ImGuizmo::TRANSLATE;
		}
	
		if (InputManager::IsKeyDownThisFrame(EKeyCode::S)) {
			this->m_currentGizmoOp = ImGuizmo::SCALE;
		}
	}
	
	const IRenderer* renderer = WindowManager::GetMainWindow()->GetInternalRenderer();
	const EditorCamera& cam = renderer->GetEditorCamera();
	// Start with translation Gizmo
	ImGuiIO& io = ImGui::GetIO();
	ImGuizmo::SetRect(0, 0, io.DisplaySize.x, io.DisplaySize.y);
	glm::mat4 viewMat = cam.GetViewMatrix();
	glm::mat4 projMat = cam.GetProjectionMatrix();
	auto* viewMatPtr = reinterpret_cast<float*>(&viewMat);
	auto* projMatPtr = reinterpret_cast<float*>(&projMat);
	WorldTransform* xform = this->m_inspectTarget->GetComponent<WorldTransform>();
	glm::mat xformMat = xform->GetTransformationMatrix();
	auto* xformMatrix = reinterpret_cast<float*>(&xformMat);
	if (ImGuizmo::Manipulate(viewMatPtr, projMatPtr, this->m_currentGizmoOp, ImGuizmo::MODE::WORLD, xformMatrix))
	{
		xform->SetTransformationMatrix(xformMat);
	}
}
