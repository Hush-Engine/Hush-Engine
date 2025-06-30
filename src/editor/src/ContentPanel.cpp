#include "ContentPanel.hpp"
#include "Query.hpp"
#include "ResourceManager.hpp"
#include "VirtualFilesystem.hpp"
#include "imgui/imgui_internal.h"
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
	this->m_fileImage = this->m_resourceManager->LoadTexture("engine_res://file.png");
}

void Hush::ContentPanel::OnRender() {
	if (ImGui::Begin("Project", nullptr, CONTENT_PANEL_FLAGS))
	{
		ImVec2 regionDimensions = ImGui::GetContentRegionAvail();
		
        float regionWidth = regionDimensions.x;

        ImGuiStyle& style = ImGui::GetStyle();
        float spacing = style.ItemSpacing.x;
        float cursorX = 0.0F;
        
		if (this->m_dirty) {
			this->RefreshDirectory();
		}
		ImGui::Text("Current Working Directory: %s", this->m_currentWorkingDirectory.c_str());
		for (const std::string& item : this->m_currentItems) {
			ImVec2 textSize = ImGui::CalcTextSize(item.c_str());
            float buttonWidth = textSize.x + style.FramePadding.x * 2.0F;

            // If this button would exceed the region width, wrap to next line
            if (cursorX + buttonWidth > regionWidth) {
                ImGui::NewLine();
                cursorX = 0.0F;
            }

            // Draw the button as a draggable source
            ImGui::PushID(item.c_str());
            if (ImGui::Button(item.c_str(), ImVec2(buttonWidth, 50.0F))) {
                // Handle click if needed
            }
            if (ImGui::BeginDragDropSource(ImGuiDragDropFlags_None)) {
                ImGui::SetDragDropPayload("CONTENT_BROWSER_ITEM", item.c_str(), item.size() + 1);
                ImGui::Text("Dragging \"%s\"", item.c_str());
                ImGui::EndDragDropSource();
            }
            ImGui::PopID();

            // Advance cursor and prepare for next same-line
            cursorX += buttonWidth + spacing;
            ImGui::SameLine(0.0f, spacing);
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
