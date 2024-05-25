#include "HushEngine.hpp"
#include <rendering/WindowManager.hpp>
#include <imgui/imgui.h>
#include <spdlog/details/os-inl.h>
#include <editor/UI.hpp>

Hush::HushEngine::~HushEngine()
{
    this->Quit();
}

void Hush::HushEngine::Run()
{
    this->m_isApplicationRunning = true;
    WindowRenderer mainRenderer(ENGINE_WINDOW_NAME.data());
    IRenderer *rendererImpl = mainRenderer.GetInternalRenderer();
    
    //Initialize any static resources we need
    UI::InitializePanels();

    while (this->m_isApplicationRunning)
    {
        std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
        mainRenderer.HandleEvents(&this->m_isApplicationRunning);
        
        //TODO: Change this to the window renderer
        if (!mainRenderer.IsActive())
        {
            // Arbitrary sleep to avoid taking all CPU usage
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
            continue;
        }
        rendererImpl->NewUIFrame();

        UI::DrawPanels();

        rendererImpl->Draw();

        std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        (void)elapsed;
    }
}

void Hush::HushEngine::Quit()
{
    this->m_isApplicationRunning = false;
}