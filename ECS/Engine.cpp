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
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS)) {
    }

    _data->window = SDL_CreateWindow("window", _data->GAME_WIDTH, _data->GAME_HEIGHT, 0);
    if (!_data->window)
        return false;

    SDL_GL_SetSwapInterval(1);

    _data->SDLrenderer = SDL_CreateRenderer(_data->window, nullptr);


    _data->inputs.Init();
    _data->renderer.SDLrenderer = _data->SDLrenderer;

    _data->state.AddState(StateRef(new StartState(_data)), 0);
    _data->state.ProcessStateChanges();

    
    return _data->SDLrenderer != nullptr;
    
}


void Engine::Update()
{
    _data->state.GetActiveState()->Update();
}


void Engine::Render()
{
    if (!_data->SDLrenderer)
    {
        std::cout << SDL_GetError() << '\n';
        return;
    }

    SDL_RenderClear(_data->SDLrenderer);

    _data->state.GetActiveState()->Render();

    SDL_RenderPresent(_data->SDLrenderer);
}

void Engine::run()
{
    const int TARGET_FPS = 60;
    const float TARGET_FRAME_TIME = 1000.0f / TARGET_FPS;
    uint64_t lastTicks = SDL_GetTicks();

    while (!_data->quit)
    {
        uint64_t currentTicks = SDL_GetTicks();
        float dt = (currentTicks - lastTicks) / 1000.0f;
        lastTicks = currentTicks;
        if (dt > 0.1f)
            dt = 0.1f;

        _data->inputs.Update(dt);
        Update();
        Render();


        uint64_t frameTicks = SDL_GetTicks() - currentTicks;
        if (frameTicks < TARGET_FRAME_TIME)
        {
            SDL_Delay((uint32_t)(TARGET_FRAME_TIME - frameTicks));
        }
    }
}
