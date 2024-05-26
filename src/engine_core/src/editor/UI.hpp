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

      private:
        template <class T> static std::unique_ptr<T> CreatePanel()
        {
            return std::make_unique<T>();
        }
        static std::vector<std::unique_ptr<IEditorPanel>> S_ACTIVE_PANELS;
    };

} // namespace Hush