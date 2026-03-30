#include "SystemSelection.hpp"
#include "Assertions.hpp"
#include "Logger.hpp"
#include "Scene.hpp"
#include "ScriptingHost.hpp"
#include "UI.hpp"

#include <imgui/imgui.h>
#include <magic_enum/magic_enum.hpp>

constexpr int32_t   MAX_COLUMNS    = 4;   // hard cap (change to 5 if preferred)
constexpr float MIN_COL_WIDTH  = 120.0f; // below this, don't add another column
constexpr float COL_GAP        = 8.0f;
constexpr float MIN_ROW_HEIGHT = 18.0f;
constexpr float MAX_ROW_HEIGHT = 28.0f;
constexpr int32_t   IDEAL_ROWS     = 6;   // target rows before spilling to a new column


bool Hush::SystemSelection::RenderSystemListWindow(Hush::Scene* scene, Hush::ScriptingHost* scriptingHost, int32_t* selectedSystemIndex) {
    HUSH_ASSERT(selectedSystemIndex != nullptr, "A pointer to the selected / desired system is needed");
	const std::vector<ScriptingSystemInfo>& items = scriptingHost->GetAvailableSystems();
    ImGui::SetNextWindowSize(ImVec2(520, 480), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSizeConstraints(ImVec2(300, 200), ImVec2(FLT_MAX, FLT_MAX));

    UI::BeginCenterPopup("System List");

    const float btnW       = 100.0f;
    const float btnSpacing = ImGui::GetStyle().ItemSpacing.x;
    const float btnBarH    = ImGui::GetFrameHeightWithSpacing();

    bool shouldKeepOpen = true;
    // --- Search bar (full width) ---
    static char searchBuf[128] = "";
    ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
    ImGui::InputTextWithHint("##search", "Search...", searchBuf, sizeof(searchBuf));
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // --- Filter ---
    // std::string query(searchBuf);
    // std::transform(query.begin(), query.end(), query.begin(), ::tolower);

    // std::vector<const SystemItem*> filtered;
    // for (auto& item : items) {
    //     std::string lower = item.name;
    //     std::transform(lower.begin(), lower.end(), lower.begin(), ::tolower);
    //     if (query.empty() || lower.find(query) != std::string::npos)
    //         filtered.push_back(&item);
    // }

    // const int   itemCount = (int)filtered.size();
    const size_t itemCount = items.size();
    const float totalW = ImGui::GetContentRegionAvail().x;
    const float gridH = ImGui::GetContentRegionAvail().y - btnBarH - ImGui::GetStyle().ItemSpacing.y;

    // --- Compute column count ---
    // Start from the number of columns needed to keep rows <= IDEAL_ROWS,
    // then clamp to MAX_COLUMNS and to what actually fits width-wise.
    int32_t cols = 1;
    if (itemCount > 0) {
        // How many columns do we need so that rows-per-col <= IDEAL_ROWS?
        cols = (int32_t)std::ceil((float)itemCount / (float)IDEAL_ROWS);
        cols = std::max(1, std::min(cols, MAX_COLUMNS));

        // Walk back if a column would be too narrow to be readable
        while (cols > 1) {
            float colW = (totalW - COL_GAP * (float)(cols - 1)) / (float)cols;
            if (colW >= MIN_COL_WIDTH) {
            	break;
            }
            --cols;
        }
    }

    const size_t rows  = (itemCount + cols - 1) / cols; // ceil(itemCount / cols)
    const float colW  = (totalW - COL_GAP * (float)(cols - 1)) / (float)cols;

    // Row height shrinks proportionally to fill the available grid height,
    // but is clamped between MIN and MAX so text stays legible.
    float rowH = (rows > 0) ? (gridH - ImGui::GetStyle().ItemSpacing.y * (float)(rows - 1)) / (float)rows
                             : MAX_ROW_HEIGHT;
    rowH = std::max(MIN_ROW_HEIGHT, std::min(rowH, MAX_ROW_HEIGHT));

    // --- Grid child (no scrollbar; everything fits by design) ---
    ImGui::BeginChild("##grid", ImVec2(0.0f, gridH), 0, ImGuiWindowFlags_NoScrollbar);

    for (size_t row = 0; row < rows; ++row) {
        for (size_t col = 0; col < cols; ++col) {
            size_t idx = col * rows + row; // column-major fill (left-to-right, top-to-bottom)
            if (idx >= itemCount) {
				break;
            }

            if (col > 0) {
                ImGui::SameLine((float)col * (colW + COL_GAP));
            }

            ImGui::PushID(items[idx].registryIndex);
            bool selected = idx == *(selectedSystemIndex);
            ImGui::Selectable(items[idx].name, &selected, 0, ImVec2(colW, rowH));
            if (selected) {
                *selectedSystemIndex = static_cast<int32_t>(idx);
            }
            ImGui::PopID();
        }
    }

    ImGui::EndChild();

    // --- Bottom-right buttons ---
    ImGui::Separator();
    ImGui::Spacing();

    ImGui::SetCursorPosX(ImGui::GetContentRegionMax().x - (btnW * 2 + btnSpacing));

    if (ImGui::Button("Cancel", ImVec2(btnW, 0))) {
        // TODO: cancel
        shouldKeepOpen = false;
    }

    ImGui::SameLine();

    ImGui::PushStyleColor(ImGuiCol_Button,        ImVec4(0.20f, 0.47f, 0.90f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.30f, 0.57f, 1.00f, 1.0f));
    ImGui::PushStyleColor(ImGuiCol_ButtonActive,  ImVec4(0.15f, 0.40f, 0.80f, 1.0f));

    if (ImGui::Button("Add System", ImVec2(btnW, 0))) {
        // TODO: add
        const ScriptingSystemInfo& systemInfo = items[*selectedSystemIndex];
        auto createResult = scriptingHost->CreateSystem(systemInfo);
        if (createResult.has_error()) {
            LogFormat(ELogLevel::Error, "Could not create system {}. Error: {}", systemInfo.name, magic_enum::enum_name(createResult.error()));
        }
        else {
            scene->AddScriptingSystem(createResult.value());
            // Maybe make a toast notification(?
        }
        shouldKeepOpen = false;
    }

    ImGui::PopStyleColor(3);

    ImGui::End();
    return shouldKeepOpen;
}

