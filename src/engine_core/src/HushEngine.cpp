#include "HushEngine.hpp"

HushEngine::~HushEngine()
{
    this->Quit();
}

void HushEngine::Run()
{
    this->m_isApplicationRunning = true;
    WindowRenderer mainRenderer(ENGINE_WINDOW_NAME.data());
    while (this->m_isApplicationRunning)
    {
        SDL_Event event;
        //TODO: Delta time calculations
        LOG_INFO_LN("Render pass");
        mainRenderer.HandleEvents(&this->m_isApplicationRunning);
        SDL_Delay(1000 / 60);
    }
}

void HushEngine::Quit()
{
    this->m_isApplicationRunning = false;
}
