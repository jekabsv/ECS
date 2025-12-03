#include "ECS.h"
#include <iostream>
#include "Engine.h"

int main()
{
    Engine engine;
    engine.Initialize();
    bool quit = false;
    SDL_Event ev;
    while (!quit) 
    {
        while (SDL_PollEvent(&ev)) 
        {
            if (ev.type == SDL_EVENT_QUIT)
                quit = true;
        }
        engine.Render();
        SDL_Delay(16);
    }

    return 0;
}