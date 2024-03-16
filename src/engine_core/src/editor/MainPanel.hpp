/*! \file MainPanel.hpp
    \author Kyn21kx
    \date 2024-03-15
    \brief Main layout panel for Hush's editor
*/

#pragma once
#include "IEditorPanel.hpp"
#include "../rendering/Renderer.hpp"
#include <imgui.h>
#include <memory>

namespace Hush
{
    class MainPanel final: public IEditorPanel
    {
    public:
        MainPanel(const IRenderer& renderer) noexcept;

        ~MainPanel() override;

        void OnRenderPass() override;

    private:
        std::unique_ptr<ImGuiContext> m_uiContext = nullptr;

    };
}
