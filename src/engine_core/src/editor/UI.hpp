/*! \file UI.hpp
    \author Kyn21kx
    \date 2024-05-25
    \brief UI methods and common components (to be moved to its own project, please)
*/

#pragma once
#include "editor/IEditorPanel.hpp"
#include <memory>
#include <unordered_map>

namespace Hush
{
    class UI
    {
      public:
        static void DrawPanels();

        static void InitializePanels();

        static bool Spinner(const char *label, float radius, int thickness,
                            const uint32_t &color = 3435973836u /*Default button color*/);

        static bool BeginToolBar();

        static void DockSpace();

      private:
        static void DrawPlayButton();
        template <class T> static std::unique_ptr<T> CreatePanel()
        {
            return std::make_unique<T>();
        }
        static std::vector<std::unique_ptr<IEditorPanel>> S_ACTIVE_PANELS;
    };

} // namespace Hush