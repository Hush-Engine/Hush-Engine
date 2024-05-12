#include "HushEngine.hpp"
#include <rendering/WindowManager.hpp>
#include <imgui/imgui.h>

Hush::HushEngine::~HushEngine()
{
    this->Quit();
}

void Hush::HushEngine::Run()
{
    this->m_isApplicationRunning = true;
    WindowRenderer mainRenderer(ENGINE_WINDOW_NAME.data());
    // Link our renderer with our ImGui implementation
    IRenderer *rendererImpl = mainRenderer.GetInternalRenderer();

    while (this->m_isApplicationRunning)
    {
        std::chrono::system_clock::time_point start = std::chrono::system_clock::now();
        // TODO: Delta time calculations
        mainRenderer.HandleEvents(&this->m_isApplicationRunning);
        rendererImpl->NewUIFrame();

        ImGui::Begin("Test");

        ImGui::Text("This is only a test... Hello there!");

        ImGui::End();

        rendererImpl->Draw();

        std::chrono::system_clock::time_point end = std::chrono::system_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::microseconds>(end - start);
        (void)elapsed;
        SDL_Delay(1000 / 60);
    }
}

void Hush::HushEngine::Quit()
{
    this->m_isApplicationRunning = false;
}