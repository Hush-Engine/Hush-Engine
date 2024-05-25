/*! \file Renderer.hpp
    \author Alan Ramirez Herrera
    \date 2024-03-03
    \brief Abstract renderer
*/

#pragma once

#include <SDL2/SDL.h>
#include <cstdint>

namespace Hush
{
    // TODO:
    class IRenderer
    {
      public:
        IRenderer(void *windowContext)
        {
            (void)windowContext;
        }

        IRenderer(const IRenderer &) = delete;
        IRenderer &operator=(const IRenderer &) = delete;
        IRenderer(IRenderer &&) = delete;
        IRenderer &operator=(IRenderer &&) = delete;

        virtual ~IRenderer() = default;

        virtual void CreateSwapChain(uint32_t width, uint32_t height) = 0;

        virtual void InitImGui() = 0;

        virtual void Draw() = 0;

        /// @brief Initializes all the internal structures needed to begin rendering, call after a swapchain has been
        /// created!
        virtual void InitRendering() = 0;

        virtual void NewUIFrame() const noexcept = 0;

        virtual void HandleEvent(const SDL_Event *event) noexcept = 0;

        [[nodiscard]] virtual void *GetWindowContext() const noexcept = 0;
    };
} // namespace Hush
