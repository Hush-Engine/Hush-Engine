/*! \file Renderer.hpp
    \author Alan Ramirez Herrera
    \date 2024-03-03
    \brief Abstract renderer
*/

#pragma once

#include <cstdint>

namespace Hush
{
    // TODO:
    class IRenderer
    {
      public:
        IRenderer(void *windowContext) : m_windowContext(windowContext)
        {
        }

        virtual ~IRenderer() = default;

        virtual void CreateSwapChain(uint32_t width, uint32_t height) = 0;

      protected:
        void *m_windowContext;
    };
} // namespace Hush
