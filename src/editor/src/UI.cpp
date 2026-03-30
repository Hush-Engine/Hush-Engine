#include "UI.hpp"
#include "CommandPanel.hpp"
#include "HierarchyPanel.hpp"
#include "InputManager.hpp"
#include "InspectorPanel.hpp"
#include "NotificationPanel.hpp"
#include "ScriptingHost.hpp"
#include "TitleBarMenuPanel.hpp"
#include "ScenePanel.hpp"
#include <cstring>
#include <glm/ext/vector_float3.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <imgui/imgui.h>
#include <imgui/imgui_internal.h>
#include "ContentPanel.hpp"

// NOLINTNEXTLINE
#define ADD_PANEL(activeScene, panelsMap, panelType) panelsMap[typeid(panelType)] = CreatePanel<panelType>(activeScene)

Hush::UI::UI()
{
	s_instance = this;
}

void Hush::UI::Init(Scene *parentScene, ScriptingHost* scriptingHost)
{
	this->SetupImGuiStyle();
	ADD_PANEL(parentScene, this->m_activePanels, TitleBarMenuPanel);
	ADD_PANEL(parentScene, this->m_activePanels, NotificationPanel);
	ADD_PANEL(parentScene, this->m_activePanels, ScenePanel);
	ADD_PANEL(parentScene, this->m_activePanels, HierarchyPanel);
	ADD_PANEL(parentScene, this->m_activePanels, ContentPanel);
	ADD_PANEL(parentScene, this->m_activePanels, CommandPanel);
	ADD_PANEL(parentScene, this->m_activePanels, InspectorPanel);
}

void Hush::UI::DrawPanels(float deltaTime)
{
	UI::DockSpace("HushDockspace", "Demo dockspace");
	UI::DrawPlayButton();
	// NOLINTNEXTLINE
	for (auto &pairEntry : this->m_activePanels)
	{
		pairEntry.second->OnRender(deltaTime);
	}
	ImGui::EndFrame();
	ImGui::Render();
}

// NOLINTBEGIN
void Hush::UI::SetupImGuiStyle()
{
	// Fork of Future Dark style from ImThemes
	ImGuiStyle &style = ImGui::GetStyle();

	style.Alpha = 1.0f;
	style.DisabledAlpha = 1.0f;
	style.WindowPadding = ImVec2(12.0f, 12.0f);
	style.WindowRounding = 0.0f;
	style.WindowBorderSize = 0.0f;
	style.WindowMinSize = ImVec2(20.0f, 20.0f);
	style.WindowTitleAlign = ImVec2(0.5f, 0.5f);
	style.WindowMenuButtonPosition = ImGuiDir_None;
	style.ChildRounding = 0.0f;
	style.ChildBorderSize = 1.0f;
	style.PopupRounding = 0.0f;
	style.PopupBorderSize = 1.0f;
	style.FramePadding = ImVec2(6.0f, 6.0f);
	style.FrameRounding = 0.0f;
	style.FrameBorderSize = 0.0f;
	style.ItemSpacing = ImVec2(12.0f, 6.0f);
	style.ItemInnerSpacing = ImVec2(6.0f, 3.0f);
	style.CellPadding = ImVec2(12.0f, 6.0f);
	style.IndentSpacing = 20.0f;
	style.ColumnsMinSpacing = 6.0f;
	style.ScrollbarSize = 12.0f;
	style.ScrollbarRounding = 0.0f;
	style.GrabMinSize = 12.0f;
	style.GrabRounding = 0.0f;
	style.TabRounding = 0.0f;
	style.TabBorderSize = 0.0f;
	style.TabMinWidthForCloseButton = 0.0f;
	style.ColorButtonPosition = ImGuiDir_Right;
	style.ButtonTextAlign = ImVec2(0.5f, 0.5f);
	style.SelectableTextAlign = ImVec2(0.0f, 0.0f);

	style.Colors[ImGuiCol_Text] = ImVec4(1.0f, 1.0f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_TextDisabled] = ImVec4(0.2745098173618317f, 0.3176470696926117f, 0.4509803950786591f, 1.0f);
	style.Colors[ImGuiCol_WindowBg] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
	style.Colors[ImGuiCol_ChildBg] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
	style.Colors[ImGuiCol_PopupBg] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
	style.Colors[ImGuiCol_Border] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
	style.Colors[ImGuiCol_BorderShadow] = ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
	style.Colors[ImGuiCol_FrameBg] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
	style.Colors[ImGuiCol_FrameBgHovered] = ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
	style.Colors[ImGuiCol_FrameBgActive] = ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
	style.Colors[ImGuiCol_TitleBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
	style.Colors[ImGuiCol_TitleBgActive] =
		ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
	style.Colors[ImGuiCol_TitleBgCollapsed] =
		ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
	style.Colors[ImGuiCol_MenuBarBg] = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarBg] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrab] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabHovered] =
		ImVec4(0.1568627506494522f, 0.168627455830574f, 0.1921568661928177f, 1.0f);
	style.Colors[ImGuiCol_ScrollbarGrabActive] =
		ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
	style.Colors[ImGuiCol_CheckMark] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_SliderGrab] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_SliderGrabActive] = ImVec4(0.5372549295425415f, 0.5529412031173706f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_Button] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
	style.Colors[ImGuiCol_ButtonHovered] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 1.0f);
	style.Colors[ImGuiCol_ButtonActive] = ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
	style.Colors[ImGuiCol_Header] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
	style.Colors[ImGuiCol_HeaderHovered] = ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 1.0f);
	style.Colors[ImGuiCol_HeaderActive] = ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
	style.Colors[ImGuiCol_Separator] = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
	style.Colors[ImGuiCol_SeparatorHovered] =
		ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
	style.Colors[ImGuiCol_SeparatorActive] = ImVec4(0.1568627506494522f, 0.1843137294054031f, 0.250980406999588f, 1.0f);
	style.Colors[ImGuiCol_ResizeGrip] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
	style.Colors[ImGuiCol_ResizeGripHovered] =
		ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 1.0f);
	style.Colors[ImGuiCol_ResizeGripActive] =
		ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
	style.Colors[ImGuiCol_Tab] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
	style.Colors[ImGuiCol_TabHovered] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
	style.Colors[ImGuiCol_TabActive] = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
	style.Colors[ImGuiCol_TabUnfocused] = ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
	style.Colors[ImGuiCol_TabUnfocusedActive] =
		ImVec4(0.0784313753247261f, 0.08627451211214066f, 0.1019607856869698f, 1.0f);
	style.Colors[ImGuiCol_PlotLines] = ImVec4(0.5215686559677124f, 0.6000000238418579f, 0.7019608020782471f, 1.0f);
	style.Colors[ImGuiCol_PlotLinesHovered] =
		ImVec4(0.03921568766236305f, 0.9803921580314636f, 0.9803921580314636f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogram] = ImVec4(1.0f, 0.2901960909366608f, 0.5960784554481506f, 1.0f);
	style.Colors[ImGuiCol_PlotHistogramHovered] =
		ImVec4(0.9960784316062927f, 0.4745098054409027f, 0.6980392336845398f, 1.0f);
	style.Colors[ImGuiCol_TableHeaderBg] =
		ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
	style.Colors[ImGuiCol_TableBorderStrong] =
		ImVec4(0.0470588244497776f, 0.05490196123719215f, 0.07058823853731155f, 1.0f);
	style.Colors[ImGuiCol_TableBorderLight] = ImVec4(0.0f, 0.0f, 0.0f, 1.0f);
	style.Colors[ImGuiCol_TableRowBg] = ImVec4(0.1176470592617989f, 0.1333333402872086f, 0.1490196138620377f, 1.0f);
	style.Colors[ImGuiCol_TableRowBgAlt] = ImVec4(0.09803921729326248f, 0.105882354080677f, 0.1215686276555061f, 1.0f);
	style.Colors[ImGuiCol_TextSelectedBg] = ImVec4(0.2352941185235977f, 0.2156862765550613f, 0.5960784554481506f, 1.0f);
	style.Colors[ImGuiCol_DragDropTarget] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_NavHighlight] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingHighlight] = ImVec4(0.4980392158031464f, 0.5137255191802979f, 1.0f, 1.0f);
	style.Colors[ImGuiCol_NavWindowingDimBg] =
		ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f);
	style.Colors[ImGuiCol_ModalWindowDimBg] =
		ImVec4(0.196078434586525f, 0.1764705926179886f, 0.5450980663299561f, 0.501960813999176f);
}

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

bool Hush::UI::InputTextWithHint(const char *label, const char *hint, char *buffer, size_t size, bool focusOnInput)
{
	char outChar = 0;
	if (focusOnInput && InputManager::FetchCharThisFrame(&outChar))
	{
		ImGui::SetKeyboardFocusHere();
	}
	return ImGui::InputTextWithHint(label, hint, buffer, size);
}

bool Hush::UI::BeginCenterPopup(const char *label, bool transparent)
{
	constexpr float transparentWindowAlpha = 0.5F;
	[[likely]]
	if (transparent)
	{
		ImGui::SetNextWindowBgAlpha(transparentWindowAlpha);
	}

	constexpr ImGuiWindowFlags windowFlags = ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoCollapse;

	const ImVec2 screenCenter = ImGui::GetMainViewport()->GetCenter();
	ImGui::SetNextWindowPos(screenCenter, ImGuiCond_Appearing, ImVec2(0.5f, 0.5f));
	ImGui::Begin(label, nullptr, windowFlags);

	return true;
}

bool Hush::UI::CustomSelectable(const char *label, bool *isHovered, ImDrawList *drawList, bool forceHover)
{
	ImVec2 textSize = ImGui::CalcTextSize(label);
	ImVec2 pos = ImGui::GetCursorScreenPos();

	// Add some padding to make the highlight look better
	float paddingX = 8.0f;
	float paddingY = 2.0f;

	// Create a rectangle that covers the text with padding
	ImRect bbox(ImVec2(pos.x - paddingX, pos.y - paddingY),
				ImVec2(pos.x + textSize.x + paddingX, pos.y + textSize.y + paddingY));

	ImGuiID id = ImGui::GetID(label);

	// Check hover state using ImGui's hoverability check
	*isHovered = forceHover || ImGui::ItemHoverable(bbox, id, ImGuiItemFlags_None);

	if (*isHovered)
	{
		drawList->AddRectFilled(bbox.Min, bbox.Max, IM_COL32(70, 70, 120, 200), // Darker blue background
								4.0f											// Rounded corners radius
		);
	}

	ImGui::InvisibleButton(label, ImVec2(textSize.x + paddingX * 2, textSize.y + paddingY * 2));

	if (*isHovered)
	{
		drawList->AddText(ImVec2(pos.x, pos.y), IM_COL32(255, 255, 0, 255), label);
	}
	else
	{
		drawList->AddText(pos, ImGui::GetColorU32(ImGuiCol_Text), label);
	}

	return *isHovered && (ImGui::IsMouseClicked(0) || ImGui::IsKeyPressed(ImGuiKey_Enter, false));
}

bool Hush::UI::BeginToolBar()
{
	constexpr ImGuiWindowFlags toolbarFlags = ImGuiWindowFlags_None;
	return ImGui::Begin("##toolbar", nullptr, toolbarFlags);
}

ImGuiID Hush::UI::DockSpace(const char *dockspaceId, const char *name, ImGuiDockNodeFlags additionalFlags)
{
	ImGuiDockNodeFlags dockspaceFlags =
		ImGuiDockNodeFlags_None | ImGuiDockNodeFlags_PassthruCentralNode | additionalFlags;

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
	ImGui::Begin(name, nullptr, window_flags);
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
