/*! \file MainPanel.hpp
    \author Kyn21kx
    \date 2024-03-15
    \brief Main layout panel for Hush's editor
*/

#pragma once
#include "../rendering/Renderer.hpp"
#include "IEditorPanel.hpp"
#include <imgui.h>
#include <memory>

namespace Hush
{
    class MainPanel final : public IEditorPanel
    {
      public:
        MainPanel(const IRenderer &renderer) noexcept;

        ~MainPanel() override;

        void OnRenderPass() override;

      private:
        ImGuiContext *m_uiContext = nullptr;
    };
} // namespace Hush
