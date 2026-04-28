#include "Engine.h"
#include <chrono>
#include <iostream>


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

    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_GAMEPAD | SDL_INIT_EVENTS))
        LOG_ERROR(GlobalLogger(), "Engine", "SDL_Init failed: " + std::string(SDL_GetError()));

    bool ttf_init;
    if (!(ttf_init = TTF_Init()))
        LOG_ERROR(GlobalLogger(), "Engine", "TTF_Init failed: " + std::string(SDL_GetError()));



    //GPU
    SDL_GPUDevice* _device = SDL_CreateGPUDevice(
        SDL_GPU_SHADERFORMAT_SPIRV | SDL_GPU_SHADERFORMAT_DXIL | SDL_GPU_SHADERFORMAT_MSL,
        true,
        NULL
    );

    if (_device == NULL) {
        SDL_Log("GPU Device creation failed: %s", SDL_GetError());
    }

    _data->device = _device;
    _data->window = SDL_CreateWindow("window", _data->GAME_WIDTH, _data->GAME_HEIGHT, 0);

    if (!_data->window)
    {
        LOG_ERROR(GlobalLogger(), "Engine", std::string("SDL_CreateWindow failed: ") + SDL_GetError());
        return false;
    }


    SDL_ClaimWindowForGPUDevice(_data->device, _data->window);
    SDL_SetGPUSwapchainParameters(_data->device, _data->window,
        SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC);

	_data->renderer.Init(_data->device, _data->window, &_data->assets, _data->GAME_WIDTH, _data->GAME_HEIGHT);




    _data->SDLrenderer = SDL_CreateRenderer(_data->window, nullptr);
    if (!_data->SDLrenderer) 
        LOG_ERROR(GlobalLogger(), "Engine", std::string("SDL_CreateRenderer failed: ") + SDL_GetError());




    _data->inputs.Init();

    _data->state.AddState(StateRef(new StartState(_data)), 0);
    _data->state.ProcessStateChanges();

    _data->spatialIndex.Init(0.f, 0.f, _data->GAME_WIDTH, _data->GAME_HEIGHT, 32);

    return 0;
    
}


void Engine::Update(float dt)
{
    auto start = std::chrono::high_resolution_clock::now();

    _data->state.GetActiveState()->ui.LayoutPass();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    //std::cout << "Ui Took: " << duration.count() << "ms" << std::endl;

    start = std::chrono::high_resolution_clock::now();

    _data->spatialIndex.Clear();
    _data->state.GetActiveState()->ecs.Run(ECS::SystemGroup::Initialise, dt); //40ms on 10k entities
	_data->spatialIndex.Build(); //30ms on 10k entities

    end = std::chrono::high_resolution_clock::now();
    duration = end - start;
    std::cout << "SpatialBuild Took: " << duration.count() << "ms" << std::endl;

    start = std::chrono::high_resolution_clock::now();

    _data->state.GetActiveState()->ecs.Run(ECS::SystemGroup::PreUpdate, dt);
    _data->state.GetActiveState()->ecs.Run(ECS::SystemGroup::Update, dt); //70ms on 10k entities 

    end = std::chrono::high_resolution_clock::now();
    duration = end - start;
    std::cout << "ECS updates Took: " << duration.count() << "ms" << std::endl;

    start = std::chrono::high_resolution_clock::now();

    _data->state.GetActiveState()->Update(dt); 

    end = std::chrono::high_resolution_clock::now();
    duration = end - start;
    //std::cout << "Update Took: " << duration.count() << "ms" << std::endl;


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
    auto start = std::chrono::high_resolution_clock::now();
    _data->renderer.StartRenderPass();

    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> duration = end - start;
    //std::cout << "Start Render Took: " << duration.count() << "ms" << std::endl;


    _data->state.GetActiveState()->ui.RenderPass();

    start = std::chrono::high_resolution_clock::now();

    _data->state.GetActiveState()->ecs.Run(ECS::SystemGroup::Render, dt);

    end = std::chrono::high_resolution_clock::now();
    duration = end - start;
    //std::cout << "ECS Render Took: " << duration.count() << "ms" << std::endl;

    _data->state.GetActiveState()->Render(dt);

    start = std::chrono::high_resolution_clock::now();


    _data->renderer.EndRenderPass();
    _data->renderer.Present();

    end = std::chrono::high_resolution_clock::now();
    duration = end - start;
    //std::cout << "End and present Took: " << duration.count() << "ms" << std::endl;
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

	LOG_INFO(GlobalLogger(), "Engine", "Starting main loop");

    while (!_data->quit)
    {
        auto framestart = std::chrono::high_resolution_clock::now();

        uint64_t currentTicks = SDL_GetTicks();
        float dt = (currentTicks - lastTicks) / 1000.0f;
        lastTicks = currentTicks;
        if (dt > 0.1f)
            dt = 0.1f;


        _data->state.ProcessStateChanges();
        
        auto start = std::chrono::high_resolution_clock::now();


        HandleInput(dt);

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> duration = end - start;
        //std::cout << "Input Took: " << duration.count() << "ms" << std::endl;

        start = std::chrono::high_resolution_clock::now();

        Update(dt);

        end = std::chrono::high_resolution_clock::now();
        duration = end - start;
        std::cout << "Update Took: " << duration.count() << "ms" << std::endl;

        start = std::chrono::high_resolution_clock::now();

        Physics(dt);

        end = std::chrono::high_resolution_clock::now();
        duration = end - start;
        //std::cout << "Physics Took: " << duration.count() << "ms" << std::endl;

        start = std::chrono::high_resolution_clock::now();

        Render(dt);

        end = std::chrono::high_resolution_clock::now();
        duration = end - start;
        std::cout << "Render Took: " << duration.count() << "ms" << std::endl;





        uint64_t frameTicks = SDL_GetTicks() - currentTicks;
        if (frameTicks < TARGET_FRAME_TIME)
        {
            SDL_Delay((uint32_t)(TARGET_FRAME_TIME - frameTicks));
        }

        end = std::chrono::high_resolution_clock::now();
        duration = end - framestart;
		std::cout << "FPS: " << 1000.0f / duration.count() << " | Total Frame Took: " << duration.count() << "ms" << '\n' << std::endl;
		//std::cout << "Total Frame Took: " << duration.count() << "ms" << '\n' << std::endl;

    }

    _data->renderer.Shutdown();
	LOG_INFO(GlobalLogger(), "Engine", "Exiting main loop");
}
