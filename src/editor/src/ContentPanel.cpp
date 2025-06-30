#include "ContentPanel.hpp"
#include "Query.hpp"
#include "ResourceManager.hpp"
#include "VirtualFilesystem.hpp"
#include <imgui/imgui.h>
#include <string_view>

constexpr ImGuiWindowFlags CONTENT_PANEL_FLAGS = ImGuiViewportFlags_NoFocusOnAppearing;

void Hush::ContentPanel::Init(Scene *activeScene) noexcept
{
	activeScene->CreateQuery<ResourceManager, VirtualFilesystem>().Each([this](Entity& entt, ResourceManager& resourceManager, VirtualFilesystem& vfs) {
		resourceManager.Init(&vfs);
		this->m_resourceManager = &resourceManager;
		this->m_filesystem = &vfs;
	});
	this->m_folderImage = this->m_resourceManager->LoadTexture("engine_res://folder.png");
}

void Hush::ContentPanel::OnRender() {
	if (ImGui::Begin("Project", nullptr, CONTENT_PANEL_FLAGS))
	{
		ImVec2 regionDimensions = ImGui::GetContentRegionAvail();
		if (this->m_dirty) {
			this->RefreshDirectory();
		}
		for (const std::string& item : this->m_currentItems) {
			ImGui::Text("%s", item.c_str());
		}
	}
	ImGui::End();
}


void Hush::ContentPanel::RefreshDirectory() {
	this->m_currentItems.clear();
	for (const std::string& item : this->m_filesystem->ListPath(this->m_currentWorkingDirectory))
	{
		this->m_currentItems.emplace_back(item);
	}
	this->m_dirty = false;
}
