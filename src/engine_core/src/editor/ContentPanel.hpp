/*! \file ContentPanel.hpp
    \author Kyn21kx
    \date 2024-05-26
    \brief Provides the UI for the content browser panel
*/

#pragma once
#include "IEditorPanel.hpp"

namespace Hush
{
    class ContentPanel final: public IEditorPanel
    {
        void OnRender() noexcept override;
    };
}