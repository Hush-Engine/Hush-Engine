#include "UI.hpp"
#include "HierarchyPanel.hpp"
#include "TitleBarMenuPanel.hpp"
#include "ScenePanel.hpp"
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "ContentPanel.hpp"

std::vector<std::unique_ptr<Hush::IEditorPanel>> Hush::UI::S_ACTIVE_PANELS;

void Hush::UI::DrawPanels()
{
    UI::DockSpace();
    UI::DrawPlayButton();
    // NOLINTNEXTLINE
    for (uint32_t i = 0; i < S_ACTIVE_PANELS.size(); i++)
    {
        S_ACTIVE_PANELS.at(i)->OnRender();
    }
}

void Hush::UI::InitializePanels()
{
    S_ACTIVE_PANELS.push_back(CreatePanel<TitleBarMenuPanel>());
    S_ACTIVE_PANELS.push_back(CreatePanel<ScenePanel>());
    S_ACTIVE_PANELS.push_back(CreatePanel<HierarchyPanel>());
    S_ACTIVE_PANELS.push_back(CreatePanel<ContentPanel>());
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
bool Hush::UI::BeginToolBar()
{
    constexpr ImGuiWindowFlags toolbarFlags = ImGuiWindowFlags_None;
    return ImGui::Begin("##toolbar", nullptr, toolbarFlags);
}
void Hush::UI::DockSpace()
{
    static ImGuiDockNodeFlags dockspace_flags = ImGuiDockNodeFlags_None;

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
    if (dockspace_flags & ImGuiDockNodeFlags_PassthruCentralNode)
        window_flags |= ImGuiWindowFlags_NoBackground;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
    ImGui::Begin("DockSpace Demo", nullptr, window_flags);
    ImGui::PopStyleVar();
    ImGui::PopStyleVar(2);

    ImGuiID dockspaceId = ImGui::GetID("HushDockspace");
    ImGui::DockSpace(dockspaceId, ImVec2(0.0f, 0.0f), dockspace_flags);
    ImGui::End();
}
void Hush::UI::DrawPlayButton()
{
    UI::BeginToolBar();
    //if (ImGui::ImageButton("PlayButton", ImVec2(20.0f, 20.0f)))
    if (ImGui::Button("PlayButton"))
    {
        //If we're on edit mode, play, otherwise, stop
    }
    ImGui::End();
}
// NOLINTEND