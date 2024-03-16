/*! \file HushEngine.hpp
    \author Kyn21kx
    \date 2024-02-28
    \brief Main class to instance our hush game engine
*/

#pragma once
#include "rendering/WindowRenderer.hpp"
#include "scripting/DotnetHost.hpp"
#include <string_view>
namespace Hush
{
    class HushEngine
    {
      public:
        /// <summary>
        /// Initializes the HushEngine with all its properties
        /// </summary>
        HushEngine() = default;

        HushEngine(const HushEngine &) = delete;
        HushEngine &operator=(const HushEngine &) = delete;

        HushEngine(HushEngine &&) noexcept = default;

        HushEngine &operator=(HushEngine &&) noexcept = default;

        ~HushEngine();

        /// <summary>
        /// Starts running the engine with UI components
        /// </summary>
        void Run();

        /// <summary>
        /// Disposes of the HushEngine
        /// </summary>
        void Quit();

      private:
        bool m_isApplicationRunning = false;
        static constexpr std::string_view ENGINE_WINDOW_NAME = "Hush Engine";
    };

} // namespace Hush
