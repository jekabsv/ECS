#pragma once
#include <SDL3/SDL.h>
#include "StateMachine.h"
#include "AssetManager.h"

struct SharedData
{
    const int GAME_WIDTH = 1920;
    const int GAME_HEIGHT = 1080;
    SDL_Window* window = nullptr;
    SDL_Renderer* renderer = nullptr;
    StateMachine state;
    AssetManager assets;

    bool quit = false;
};

typedef std::shared_ptr<SharedData> SharedDataRef;