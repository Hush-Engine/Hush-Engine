/*! \file IEditorPanel.hpp
    \author Kyn21kx
    \date 2024-03-15
    \brief Interface describing the behaviour of editor panel layouts for any purpose
*/

#pragma once
#include "rendering/Renderer.hpp"

namespace Hush
{

    class IEditorPanel
    {
      public:
        IEditorPanel() noexcept = default;

        IEditorPanel(const IEditorPanel &other) = default;

        IEditorPanel(IEditorPanel &&other) = default;

        IEditorPanel &operator=(const IEditorPanel &) = default;

        IEditorPanel &operator=(IEditorPanel &&) = default;

        virtual ~IEditorPanel() = default;

        virtual void OnRender() = 0;
    };

} // namespace Hush