/*! \file ApplicationLoader.hpp
\author Alan Ramirez
    \date 2024-09-21
    \brief Helper to load an application in hush
*/

#pragma once

#include "IApplication.hpp"
#include <memory>

namespace Hush
{
    /// Loads an application. The method to load an application depends on each platform and if shared library loading
    /// is enabled.
    ///
    /// If HUSH_STATIC_APP definition is set to true, Hush won't try to load an application hosted in a shared library.
    /// This only applies on platforms that support shared libraries.
    ///
    /// If a static application is bundled with the engine, it won't attempt to load a shared library.
    ///
    /// @return A pointer to the loaded application.
    std::unique_ptr<IApplication> LoadApplication();
} // namespace Hush
