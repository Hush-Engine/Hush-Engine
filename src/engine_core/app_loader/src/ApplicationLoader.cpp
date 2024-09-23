/*! \file ApplicationLoader.cpp
\author Alan Ramirez
    \date 2024-09-21
    \brief Helper to load an application in hush
*/

#include "ApplicationLoader.hpp"
#include "AppSupport.hpp"
#include "Platform.hpp"

#include <Logger.hpp>

class DummyApplication final : public Hush::IApplication
{
    public:

    DummyApplication() : IApplication("ERROR: no app loaded")
    {
    }

    ~DummyApplication() override = default;

    void Init() override
    {
        Log(Hush::ELogLevel::Error, "No application was loaded");
    }

    void Update() override
    {
    }

    void OnRender() override
    {
    }

    void OnPostRender() override
    {
    }

    void OnPreRender() override
    {
    }
};

extern "C" bool BundledAppExists_Internal_() HUSH_WEAK;

extern "C" Hush::IApplication* BundledApp_Internal_() HUSH_WEAK;

std::unique_ptr<Hush::IApplication> Hush::LoadApplication()
{
    // First, check if platform supports shared library app. If not, just attempt to load the bundled app.
#if !HUSH_SUPPORTS_SHARED_APP
    return BundledApp__Internal();
#else
    // Ok, we support apps as shared libraries, we then must check if a bundled application exists.
    if (BundledAppExists_Internal_())
    {
        // It exists, just return it.
        return std::unique_ptr<IApplication>(BundledApp_Internal_());
    }

    // We can't find it, attempt to load it through a shared library.
    // TODO: define file metadata????

    return nullptr;
#endif
}