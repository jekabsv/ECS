#pragma once
#include <SDL3/SDL.h>
#include "StateMachine.h"
#include "AssetManager.h"
#include "InputSystem.h"


struct SharedData
{
    const int GAME_WIDTH = 1920;
    const int GAME_HEIGHT = 1080;
    SDL_Window* window = nullptr;
    SDL_Renderer* SDLrenderer = nullptr;
    StateMachine state;
    AssetManager assets;
    InputSystem::System inputs;
    Renderer renderer;
    bool quit = false;
};

typedef std::shared_ptr<SharedData> SharedDataRef;