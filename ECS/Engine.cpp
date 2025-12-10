#include "Engine.h"


#ifdef __EMSCRIPTEN__
bool Engine::InitializeWeb(const std::string& elementId)
{
    if (SDL_Init(SDL_INIT_VIDEO) != 0)
        return false;

    SDL_SetHint(SDL_HINT_EMSCRIPTEN_KEYBOARD_ELEMENT, elementId.c_str());

    window = SDL_CreateWindow("WASM", 0, 0, GAME_WIDTH, GAME_HEIGHT, SDL_WINDOW_SHOWN);
    if (!window)
        return false;

#ifdef __EMSCRIPTEN__
    if (!mainLoopRegistered) {
        emscripten_set_main_loop_arg([](void* arg) {static_cast<Engine*>(arg)->Render(); }, this, 0, false);
        mainLoopRegistered = true;
    }
    SDL_GL_SetSwapInterval(1);
#endif

    renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
#ifdef __EMSCRIPTEN__
    emscripten_set_main_loop_timing(1, 1);
#endif
    return renderer != nullptr;
}
#endif // __EMSCRIPTEN__


bool Engine::Initialize()
{
    
    _data->window = SDL_CreateWindow("window", _data->GAME_WIDTH, _data->GAME_HEIGHT, 0);
    if (!_data->window)
        return false;

    SDL_GL_SetSwapInterval(1);

    _data->renderer = SDL_CreateRenderer(_data->window, nullptr);



    
    _data->state.AddState(StateRef(new StartState(_data)), 0);
    _data->state.ProcessStateChanges();
    return _data->renderer != nullptr;
}


void Engine::Update()
{
    _data->state.GetActiveState()->Update();
}


void Engine::Render()
{
    _data->state.GetActiveState()->Render();
}

void Engine::run()
{
    while (!_data->quit)
    {
        Update();
        Render();
        SDL_Delay(16);
    }
}
