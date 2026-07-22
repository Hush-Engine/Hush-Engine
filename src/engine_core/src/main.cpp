#include "HushEngine.hpp"
#include "Logger.hpp"
#include "Scene.hpp"

#if defined(HUSH_USE_MIMALLOC)
#include <mimalloc.h>
#endif
#define SDL_MAIN_USE_CALLBACKS
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_init.h>

namespace
{
	struct AppState
	{
		Hush::HushEngine engine;
	};
} // namespace

extern "C"
{
	SDL_AppResult SDL_AppInit(void **appstate, int argc, char **argv)
	{
#if defined(HUSH_USE_MIMALLOC) && HUSH_PLATFORM_WIN
		const int miV = mi_version();
		const bool redirected = mi_is_redirected();
		Hush::LogFormat(redirected ? Hush::ELogLevel::Info : Hush::ELogLevel::Warn,
						"mimalloc {}.{}.{} active (redirect={})", miV / 1000, (miV / 100) % 10, miV % 100,
						redirected ? "ok" : "FAILED - third-party DLLs use CRT malloc");
#endif
		*appstate = new AppState;
		AppState &state = *static_cast<AppState *>(*appstate);

		if (!SDL_InitSubSystem(SDL_INIT_VIDEO | SDL_INIT_EVENTS))
		{
			Hush::LogFormat(Hush::ELogLevel::Error, "SDL_InitSubSystem failed: {}", SDL_GetError());
			return SDL_APP_FAILURE;
		}

		state.engine.Init(argc, argv);

		return SDL_APP_CONTINUE;
	}

	SDL_AppResult SDL_AppIterate(void *appstate)
	{
		AppState &state = *static_cast<AppState *>(appstate);
		state.engine.Run();

		return SDL_APP_CONTINUE;
	}

	SDL_AppResult SDL_AppEvent(void *appstate, SDL_Event *event)
	{
		AppState &state = *static_cast<AppState *>(appstate);
		state.engine.HandleEvents(*event);

		if (!state.engine.ShouldRun()) {
			return SDL_APP_SUCCESS;
		}

		return SDL_APP_CONTINUE;
	}

	void SDL_AppQuit(void *appstate, [[maybe_unused]] SDL_AppResult result)
	{
		AppState &state = *static_cast<AppState *>(appstate);
		state.engine.Quit();

		delete &state;
	}
}
