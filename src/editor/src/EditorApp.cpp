//
// Created by Alan5 on 22/09/2024.
//

#include "IApplication.hpp"
#include "UI.hpp"

#include <memory>

class EditorApp final : public Hush::IApplication
{
  public:
    EditorApp() : IApplication("Hush-Editor")
    {
    }

    EditorApp(const EditorApp&) = delete;
    EditorApp(EditorApp&&) = delete;
    EditorApp& operator=(const EditorApp&) = delete;
    EditorApp& operator=(EditorApp&&) = delete;

    ~EditorApp() override = default;

    void Init() override
    {
    }

    void Update() override
    {

    }

    void OnRender() override
    {
        Hush::UI::DrawPanels();
    }

    void OnPostRender() override
    {

    }

    void OnPreRender() override
    {
    }
};

extern "C" bool BundledAppExists_Internal_() // NOLINT(*-identifier-naming)
{
    return true;
}

extern "C" Hush::IApplication* BundledApp_Internal_() // NOLINT(*-identifier-naming)
{
    return  new EditorApp();
}
