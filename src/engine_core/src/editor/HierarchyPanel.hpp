/*! \file HierarchyPanel.hpp
    \author Kyn21kx
    \date 2024-05-24
    \brief Panel to display objects loaded to a scene
*/

#pragma once
#include "editor/IEditorPanel.hpp"
namespace Hush
{
    class HierarchyPanel final : public IEditorPanel
    {
      public:
        void OnRender() override;
    };
} // namespace Hush