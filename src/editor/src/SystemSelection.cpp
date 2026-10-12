#include "SystemSelection.hpp"
#include "Assertions.hpp"
#include "HushEngine.hpp"
#include "Logger.hpp"
#include "Scene.hpp"
#include "ModuleRegistry.hpp"
#include "UI.hpp"
#include "UIUtils.hpp"

#include <imgui/imgui.h>
#include <algorithm>
#include <string>
#include <vector>

bool Hush::SystemSelection::RenderSystemListWindow(Hush::Scene *scene, State *state)
{
	HUSH_ASSERT(state != nullptr, "A valid SystemSelection::State is needed");
	Modules::ModuleRegistry *registry = scene->GetEngine()->GetModuleRegistry();
	const auto &modules = registry->GetAllModules();

	ImGui::SetNextWindowSize(ImVec2(420, 480), ImGuiCond_FirstUseEver);
	ImGui::SetNextWindowSizeConstraints(ImVec2(280, 200), ImVec2(FLT_MAX, FLT_MAX));

	UI::BeginCenterPopup("System List");

	const float btnW = 100.0f;
	const float btnSpacing = ImGui::GetStyle().ItemSpacing.x;
	const float btnBarH = ImGui::GetFrameHeightWithSpacing() + ImGui::GetStyle().ItemSpacing.y;

	bool shouldKeepOpen = true;

	// Search bar
	static char searchBuf[128] = "";
	ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
	ImGui::InputTextWithHint("##search", "Search...", searchBuf, sizeof(searchBuf));
	ImGui::Spacing();
	ImGui::Separator();
	ImGui::Spacing();

	const std::string_view search = searchBuf;

	// Pre-filter: collect visible systems, skipping non-Active modules and non-matching names.
	// moduleName string_views point into record.name (std::string) — null-terminated, safe for ImGui.
	struct VisibleEntry
	{
		ModuleHandle       module;
		std::string_view   moduleName;
		Reflection::TypeId typeId;
		std::string        name; // owned — ForeignSystemDescriptor::name may not be null-terminated
	};

	std::vector<VisibleEntry> visible;
	for (const auto &[handle, record] : modules)
	{
		if (record.state != Modules::EModuleState::Active)
			continue;

		for (const auto &sys : record.foreignSystems)
		{
			if (!search.empty() && sys.name.find(search) == std::string_view::npos)
				continue;
			visible.push_back({handle, record.name, sys.typeId, std::string(sys.name)});
		}
		for (const auto &sys : record.nativeSystems)
		{
			std::string_view sysName = sys.name ? sys.name : "";
			if (!search.empty() && sysName.find(search) == std::string_view::npos)
				continue;
			visible.push_back({handle, record.name, sys.typeId, std::string(sysName)});
		}
	}

	// Sort by module name so a single pass can group entries under headers.
	std::sort(visible.begin(), visible.end(), [](const VisibleEntry &a, const VisibleEntry &b) {
		return a.moduleName < b.moduleName;
	});

	// Scrollable list
	const float listH = ImGui::GetContentRegionAvail().y - btnBarH;
	ImGui::BeginChild("##syslist", ImVec2(0.0f, listH), 0, ImGuiWindowFlags_None);

	std::string_view currentModule;
	bool headerOpen = false;
	for (const auto &entry : visible)
	{
		if (entry.moduleName != currentModule)
		{
			if (headerOpen) {
				ImGui::TreePop();
			}
			currentModule = entry.moduleName;
			headerOpen = ImGui::TreeNodeEx(currentModule.data(),
                ImGuiTreeNodeFlags_DefaultOpen | ImGuiTreeNodeFlags_SpanAvailWidth);
		}
		if (!headerOpen) {
			continue;
		}

		bool isChecked = state->checked.contains(entry.typeId.id);
		ImGui::PushID((int)(entry.typeId.id & 0xFFFFFFFFu));
		if (ImGui::Checkbox(entry.name.c_str(), &isChecked))
		{
			if (isChecked) {
				state->checked.insert(entry.typeId.id);
			}
			else {
				state->checked.erase(entry.typeId.id);
			}
		}
		ImGui::PopID();
	}
	if (headerOpen) {
		ImGui::TreePop();
	}

	ImGui::EndChild();

	// Bottom buttons
	ImGui::Separator();
	ImGui::Spacing();

	const bool anyChecked = !state->checked.empty();

	ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - (btnW * 2.0f + btnSpacing));

	if (ImGui::Button("Cancel", ImVec2(btnW, 0)))
	{
		state->Clear();
		shouldKeepOpen = false;
	}

	ImGui::SameLine();

	ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.20f, 0.47f, 0.90f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.57f, 1.00f, 1.0f));
	ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.15f, 0.40f, 0.80f, 1.0f));

	if (!anyChecked)
		ImGui::BeginDisabled();

	// PERF(Nef): This is still quite bad, we could make it better by saving both the module handle and the system id in our set
	if (ImGui::Button("Add System", ImVec2(btnW, 0)))
	{
		for (const auto &entry : visible)
		{
			if (!state->checked.contains(entry.typeId.id)) {
				continue;
			}

			const auto err = registry->AddSystemToScene(entry.module, entry.typeId, *scene);
			if (err != Modules::ModuleRegistry::EError::None)
			{
				LogFormat(ELogLevel::Error, "Could not add system {}", entry.name);
			}
			else
			{
				Entity toast = scene->CreateEntity();
				toast.EmplaceComponent<ToastNotification>(
					std::string("Created system: ") + entry.name, 2.f,
					ToastNotification::EToastType::Info);
			}
		}
		state->Clear();
		shouldKeepOpen = false;
	}

	if (!anyChecked) {
		ImGui::EndDisabled();
	}

	ImGui::PopStyleColor(3);
	ImGui::End();
	return shouldKeepOpen;
}
