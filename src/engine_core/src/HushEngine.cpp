#include "HushEngine.hpp"
// #include <editor/UI.hpp>
#include "ApplicationLoader.hpp"
#include <WindowManager.hpp>
#include <imgui/imgui.h>
#include <spdlog/details/os-inl.h>

Hush::HushEngine::~HushEngine()
{
    this->Quit();
}

void Hush::HushEngine::Run()
{
    this->m_app = LoadApplication();

    this->m_isApplicationRunning = true;
    WindowRenderer mainRenderer(m_app->GetAppName().c_str());
    IRenderer *rendererImpl = mainRenderer.GetInternalRenderer();

    // Initialize any static resources we need
    this->Init();

    this->m_app->Init();

    while (this->m_isApplicationRunning)
    {
        std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
        mainRenderer.HandleEvents(&this->m_isApplicationRunning);
        // TODO: Change this to the window renderer
        if (!mainRenderer.IsActive())
        {
            // Arbitrary sleep to avoid taking all CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }

        this->m_app->Update();

        this->m_app->OnPreRender();

        rendererImpl->NewUIFrame();

        this->m_app->OnRender();

        // UI::DrawPanels();

        rendererImpl->Draw();

        this->m_app->OnPostRender();

        std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        (void)elapsed;
    }
}

void Hush::HushEngine::Quit()
{
    this->m_isApplicationRunning = false;
}

void Hush::HushEngine::Init()
{
}
