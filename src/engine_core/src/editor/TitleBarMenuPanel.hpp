/*! \file TitleBarMenuPanel.hpp
    \author Kyn21kx
    \date 2024-05-25
    \brief The main title bar menu for the Hush Engine
*/

#pragma once
#include "editor/IEditorPanel.hpp"
namespace Hush
{
    class TitleBarMenuPanel final : public IEditorPanel
    {
      public:
        void OnRender() noexcept override;
    };
} // namespace Hush