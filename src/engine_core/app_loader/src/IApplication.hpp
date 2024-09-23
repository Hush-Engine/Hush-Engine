/*! \file IApplication.hpp
\author Alan Ramirez
    \date 2024-09-21
    \brief Application meant to be run by Hush
*/

#pragma once

#include <string>
#include <string_view>

namespace Hush
{
    class IApplication
    {
      public:
        IApplication(std::string_view appName) : m_appName(appName)
        {
        }

        IApplication(const IApplication &) = delete;
        IApplication(IApplication &&) = delete;
        IApplication &operator=(const IApplication &) = default;
        IApplication &operator=(IApplication &&) = default;

        virtual ~IApplication() = default;

        virtual void Init() = 0;

        virtual void Update() = 0;

        virtual void OnPreRender() = 0;

        virtual void OnRender() = 0;

        virtual void OnPostRender() = 0;

        [[nodiscard]] std::string GetAppName() const
        {
            return m_appName;
        }

    private:
        std::string m_appName;
    };
} // namespace Hush
