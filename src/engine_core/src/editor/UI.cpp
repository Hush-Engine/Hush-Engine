#include "UI.hpp"
#include "HierarchyPanel.hpp"
#include "TitleBarMenuPanel.hpp"

std::vector<std::unique_ptr<Hush::IEditorPanel>> Hush::UI::S_ACTIVE_PANELS;

void Hush::UI::DrawPanels()
{
    // NOLINTNEXTLINE
    for (uint32_t i = 0; i < S_ACTIVE_PANELS.size(); i++)
    {
        S_ACTIVE_PANELS.at(i)->OnRender();
    }
}

void Hush::UI::InitializePanels()
{
    S_ACTIVE_PANELS.push_back(CreatePanel<HierarchyPanel>());
    S_ACTIVE_PANELS.push_back(CreatePanel<TitleBarMenuPanel>());
}
