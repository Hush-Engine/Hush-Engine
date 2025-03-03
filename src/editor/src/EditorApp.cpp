//
// Created by Alan5 on 22/09/2024.
//

#include "IApplication.hpp"
#include "UI.hpp"

#include <memory>

class EditorApp final : public Hush::IApplication
{
public:
    EditorApp(Hush::HushEngine *engine)
        : m_scene(std::make_unique<Hush::Scene>(engine))
    {
    }

    EditorApp(const EditorApp &) = delete;
    EditorApp(EditorApp &&) = delete;
    EditorApp &operator=(const EditorApp &) = delete;
    EditorApp &operator=(EditorApp &&) = delete;

    ~EditorApp() override = default;

    void Init() override
    {
        this->m_scene->Init();
    }

    void Update(float delta) override
    {
        this->m_scene->Update(delta);
    }

    void FixedUpdate(float delta) override
    {
        this->m_scene->FixedUpdate(delta);
    }

    void OnRender() override
    {
        this->m_scene->Render();
        this->userInterface.DrawPanels();
    }

    void OnPostRender() override
    {
        this->m_scene->PostRender();
    }

    void OnPreRender() override
    {
        this->m_scene->PreRender();
    }

    std::string_view GetAppName() const noexcept override
    {
        return "Hush-Editor";
    }

private:
    Hush::UI userInterface;
    std::unique_ptr<Hush::Scene> m_scene;
};

extern "C" bool BundledAppExists_Internal_() // NOLINT(*-identifier-naming)
{
    return true;
}

extern "C" Hush::IApplication *BundledApp_Internal_(Hush::HushEngine *engine) // NOLINT(*-identifier-naming)
{
    return new EditorApp(engine);
}
