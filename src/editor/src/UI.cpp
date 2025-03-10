#include "UI.hpp"
#include "CommandPanel.hpp"
#include "HierarchyPanel.hpp"
#include "InspectorPanel.hpp"
#include "TitleBarMenuPanel.hpp"
#include "ScenePanel.hpp"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "ContentPanel.hpp"

#define ADD_PANEL(activeScene, panelsMap, panelType) panelsMap[typeid(panelType)] = CreatePanel<panelType>(activeScene)

Hush::UI::UI()
{
	s_instance = this;
}

void Hush::UI::Init(Scene* parentScene) {
	ADD_PANEL(parentScene, this->m_activePanels, TitleBarMenuPanel);
	ADD_PANEL(parentScene, this->m_activePanels, TitleBarMenuPanel);
	ADD_PANEL(parentScene, this->m_activePanels, ScenePanel);
	ADD_PANEL(parentScene, this->m_activePanels, HierarchyPanel);
	ADD_PANEL(parentScene, this->m_activePanels, ContentPanel);
	// ADD_PANEL(parentScene, this->m_activePanels, DebugUI);
	// ADD_PANEL(parentScene, this->m_activePanels, DebugTooltip);
	// ADD_PANEL(parentScene, this->m_activePanels, StatsPanel);
	ADD_PANEL(parentScene, this->m_activePanels, CommandPanel);
	ADD_PANEL(parentScene, this->m_activePanels, InspectorPanel);
}

void Hush::UI::DrawPanels()
{
	UI::DockSpace("HushDockspace", "Demo dockspace");
	UI::DrawPlayButton();
	// NOLINTNEXTLINE
	for (auto &pairEntry : this->m_activePanels)
	{
		pairEntry.second->OnRender();
	}
	ImGui::EndFrame();
	ImGui::Render();
}

// NOLINTBEGIN
#pragma warning(push, 0)
bool Hush::UI::Spinner(const char *label, float radius, int thickness, const uint32_t &color)
{
	ImGuiWindow *window = ImGui::GetCurrentWindow();
	if (window->SkipItems)
	{
		return false;
	}

	ImGuiContext &g = *GImGui;
	const ImGuiStyle &style = g.Style;
	const ImGuiID id = window->GetID(label);

	ImVec2 pos = window->DC.CursorPos;
	ImVec2 size((radius) * 2, (radius + style.FramePadding.y) * 2);

	const ImRect bb(pos, ImVec2(pos.x + size.x, pos.y + size.y));
	ImGui::ItemSize(bb, style.FramePadding.y);
	if (!ImGui::ItemAdd(bb, id))
	{
		return false;
	}

	// Render
	window->DrawList->PathClear();

	int numSegments = 30;
	int start = abs(ImSin(g.Time * 1.8f) * ((float)numSegments - 5));

	const float a_min = IM_PI * 2.0f * ((float)start) / (float)numSegments;
	const float a_max = IM_PI * 2.0f * ((float)numSegments - 3) / (float)numSegments;

	const ImVec2 centre = ImVec2(pos.x + radius, pos.y + radius + style.FramePadding.y);

	for (int i = 0; i < numSegments; i++)
	{
		const float a = a_min + ((float)i / (float)numSegments) * (a_max - a_min);
		window->DrawList->PathLineTo(
			ImVec2(centre.x + ImCos(a + g.Time * 8) * radius, centre.y + ImSin(a + g.Time * 8) * radius));
	}

	window->DrawList->PathStroke(color, 0, thickness);
	return true;
}


bool Hush::UI::CustomSelectable(const char* label, bool* isHovered, ImDrawList* drawList, bool forceHover) {
	
        ImVec2 textSize = ImGui::CalcTextSize(label);
        ImVec2 pos = ImGui::GetCursorScreenPos();
        
        // Add some padding to make the highlight look better
        float paddingX = 8.0f;
        float paddingY = 2.0f;
        
        // Create a rectangle that covers the text with padding
        ImRect bbox(
            ImVec2(pos.x - paddingX, pos.y - paddingY),
            ImVec2(pos.x + textSize.x + paddingX, pos.y + textSize.y + paddingY)
        );
        
        ImGuiID id = ImGui::GetID(label);
        
        // Check hover state using ImGui's hoverability check
        *isHovered = forceHover || ImGui::ItemHoverable(bbox, id, ImGuiItemFlags_None);

        if (*isHovered) {	
	        drawList->AddRectFilled(
	            bbox.Min, 
	            bbox.Max, 
	            IM_COL32(70, 70, 120, 200), // Darker blue background
	            4.0f  // Rounded corners radius
	        );
        }

        ImGui::InvisibleButton(label, ImVec2(textSize.x + paddingX * 2, textSize.y + paddingY * 2));
        
        if (*isHovered) {
            drawList->AddText(ImVec2(pos.x, pos.y), IM_COL32(255, 255, 0, 255), label);
        } else {
            drawList->AddText(pos, ImGui::GetColorU32(ImGuiCol_Text), label);
        }
		
        
        return *isHovered && (ImGui::IsMouseClicked(0) || ImGui::IsKeyPressed(ImGuiKey_Enter, false));
}

bool Hush::UI::BeginToolBar()
{
	constexpr ImGuiWindowFlags toolbarFlags = ImGuiWindowFlags_None;
	return ImGui::Begin("##toolbar", nullptr, toolbarFlags);
}

ImGuiID Hush::UI::DockSpace(const char* dockspaceId, const char* name, ImGuiDockNodeFlags additionalFlags)
{
	ImGuiDockNodeFlags dockspaceFlags = ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_PassthruCentralNode | additionalFlags;

	// We are using the ImGuiWindowFlags_NoDocking flag to make the parent window not dockable into,
	// because it would be confusing to have two docking targets within each others.
	ImGuiWindowFlags window_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;

	const ImGuiViewport *viewport = ImGui::GetMainViewport();
	ImGui::SetNextWindowPos(viewport->WorkPos);
	ImGui::SetNextWindowSize(viewport->WorkSize);
	ImGui::SetNextWindowViewport(viewport->ID);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
	ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
	window_flags |=
		ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove;
	window_flags |= ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus;

	// When using ImGuiDockNodeFlags_PassthruCentralNode, DockSpace() will render our background
	// and handle the pass-thru hole, so we ask Begin() to not render a background.
	if (dockspaceFlags & ImGuiDockNodeFlags_PassthruCentralNode)
	{
		window_flags |= ImGuiWindowFlags_NoBackground;
	}

	ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
	ImGui::Begin("DockSpace Demo", nullptr, window_flags);
	ImGui::PopStyleVar();
	ImGui::PopStyleVar(2);

	ImGuiID imguiDockspaceId = ImGui::GetID(dockspaceId);
	ImGui::DockSpace(imguiDockspaceId, ImVec2(0.0f, 0.0f), dockspaceFlags);
	ImGui::End();
	return imguiDockspaceId;
}

Hush::UI &Hush::UI::Get()
{
	return *s_instance;
}

void Hush::UI::DrawPlayButton()
{
	UI::BeginToolBar();
	// if (ImGui::ImageButton("PlayButton", ImVec2(20.0f, 20.0f)))
	if (ImGui::Button("PlayButton"))
	{
		// If we're on edit mode, play, otherwise, stop
	}
	ImGui::End();
}
// NOLINTEND
