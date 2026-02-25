#include "HushEngine.hpp"
#include "Scene.hpp"

#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_init.h>

#include <flecs.h>

namespace
{
    struct AppState
    {
        Hush::HushEngine engine;
    };
}

// int main()
// {
// 	Hush::HushEngine engine;
// 	engine.Run();
// 	engine.Quit();
// 	return 0;
// }

extern "C" {

SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
{
    *appstate = new AppState;
    AppState& state = *static_cast<AppState*>(*appstate);

    if(!SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
    {
        Hush::LogFormat(Hush::ELogLevel::Error, "SDL_InitSubSystem failed: {}", SDL_GetError());
        return SDL_APP_FAILURE;
    }

    state.engine.Init();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppIterate(void *appstate)
{
    AppState& state = *static_cast<AppState*>(appstate);
    state.engine.Run();

    return SDL_APP_CONTINUE;
}

SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
{
    AppState& state = *static_cast<AppState*>(appstate);
    state.engine.HandleEvents(*event);

    return SDL_APP_CONTINUE;
}

void SDL_AppQuit(void *appstate, SDL_AppResult result)
{
    AppState& state = *static_cast<AppState*>(appstate);
    state.engine.Quit();

    delete &state;
}

}
