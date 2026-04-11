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
    GlobalLogger().AddSink(std::make_shared<ConsoleSink>());

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS)) {
        LOG_ERROR(GlobalLogger(), "Engine", "SDL_Init failed");
    }

    TTF_Init();


    _data->window = SDL_CreateWindow("window", _data->GAME_WIDTH, _data->GAME_HEIGHT, 0);
    if (!_data->window)
    {
        LOG_ERROR(GlobalLogger(), "Engine", std::string("SDL_CreateWindow failed: ") + SDL_GetError());
        return false;

    }

    SDL_GL_SetSwapInterval(1);

    _data->SDLrenderer = SDL_CreateRenderer(_data->window, nullptr);
    if (!_data->SDLrenderer) 
        LOG_ERROR(GlobalLogger(), "Engine", std::string("SDL_CreateRenderer failed: ") + SDL_GetError());

    _data->inputs.Init();
    _data->renderer.SDLrenderer = _data->SDLrenderer;

    _data->state.AddState(StateRef(new StartState(_data)), 0);
    _data->state.ProcessStateChanges();
    

    _data->spatialIndex.Init(0.f, 0.f, _data->GAME_WIDTH, _data->GAME_HEIGHT, 32);


    return _data->SDLrenderer != nullptr;
    
}


void Engine::Update(float dt)
{
    _data->state.GetActiveState()->ui.LayoutPass();

    _data->spatialIndex.Clear();
    _data->state.GetActiveState()->ecs.Run(ECS::SystemGroup::Initialise, dt);
    _data->spatialIndex.Build();

    _data->state.GetActiveState()->ecs.Run(ECS::SystemGroup::PreUpdate, dt);
    _data->state.GetActiveState()->ecs.Run(ECS::SystemGroup::Update, dt);
    _data->state.GetActiveState()->Update(dt);
    _data->state.GetActiveState()->ecs.Run(ECS::SystemGroup::PostUpdate, dt);
}

void Engine::HandleInput(float dt)
{
    UI::InputState inp;

    SDL_Event ev;
    while (SDL_PollEvent(&ev))
    {
        _data->inputs.HandleEvent(ev, _data->quit);

        if (ev.type == SDL_EVENT_TEXT_INPUT)
            inp.textInput += ev.text.text;

        if (ev.type == SDL_EVENT_KEY_DOWN)
        {
            switch (ev.key.scancode)
            {
            case SDL_SCANCODE_BACKSPACE: inp.keyBackspace = true; break;
            case SDL_SCANCODE_DELETE:    inp.keyDelete = true; break;
            case SDL_SCANCODE_LEFT:      inp.keyLeft = true; break;
            case SDL_SCANCODE_RIGHT:     inp.keyRight = true; break;
            case SDL_SCANCODE_HOME:      inp.keyHome = true; break;
            case SDL_SCANCODE_END:       inp.keyEnd = true; break;
            case SDL_SCANCODE_RETURN:    inp.keyEnter = true; break;
            case SDL_SCANCODE_TAB:       inp.keyTab = true; break;
            default: break;
            }
        }
    }

    _data->inputs.Update(dt);

    inp.mouseX = _data->inputs.GetMousePosition().x;
    inp.mouseY = _data->inputs.GetMousePosition().y;
    inp.mouseDown = _data->inputs.GetActionState("click") == InputSystem::Held || _data->inputs.GetActionState("click") == InputSystem::Pressed;
    inp.mousePressed = _data->inputs.GetActionState("click") == InputSystem::Pressed;
    inp.mouseReleased = _data->inputs.GetActionState("click") == InputSystem::Released;

    _data->state.GetActiveState()->ui.Update(inp, dt);
}

void Engine::Render(float dt)
{
    if (!_data->SDLrenderer)
    {
        LOG_ERROR(GlobalLogger(), "Renderer", std::string("Render failed: ") + SDL_GetError());
        return;
    }

	SDL_SetRenderDrawColor(_data->SDLrenderer, 0, 0, 0, 255);
    SDL_RenderClear(_data->SDLrenderer);

    _data->state.GetActiveState()->ui.RenderPass();
    _data->state.GetActiveState()->ecs.Run(ECS::SystemGroup::Render, dt);
    _data->state.GetActiveState()->Render(dt);

    SDL_RenderPresent(_data->SDLrenderer);
}

void Engine::Physics(float dt)
{
	_data->state.GetActiveState()->ecs.Run(ECS::SystemGroup::Physics, dt);
}




void Engine::run()
{
    const int TARGET_FPS = 120;
    const float TARGET_FRAME_TIME = 1000.0f / TARGET_FPS;
    uint64_t lastTicks = SDL_GetTicks();

    while (!_data->quit)
    {
        uint64_t currentTicks = SDL_GetTicks();
        float dt = (currentTicks - lastTicks) / 1000.0f;
        lastTicks = currentTicks;
        if (dt > 0.1f)
            dt = 0.1f;



        _data->state.ProcessStateChanges();
        
        HandleInput(dt);
        Update(dt);
        Physics(dt);
        Render(dt);



        uint64_t frameTicks = SDL_GetTicks() - currentTicks;
        if (frameTicks < TARGET_FRAME_TIME)
        {
            SDL_Delay((uint32_t)(TARGET_FRAME_TIME - frameTicks));
        }
    }
}
