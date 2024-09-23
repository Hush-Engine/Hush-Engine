/*! \file WindowManager.hpp
    \author Kyn21kx
    \date 2024-04-27
    \brief Global references to the available windows
*/

#pragma once
#include "WindowRenderer.hpp"
#include "Logger.hpp"

namespace Hush
{
    class WindowManager
    {
      public:
        static WindowRenderer *GetMainWindow()
        {
            return s_windowRenderer;
        }

        static void SetMainWindow(WindowRenderer *window)
        {
            if (s_windowRenderer != nullptr)
            {
                LogWarn("Cannot override main window!");
            }
            s_windowRenderer = window;
        }

      private:
        static inline WindowRenderer *s_windowRenderer = nullptr;
    };
} // namespace Hush
