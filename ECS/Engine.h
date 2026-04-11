#pragma once
#include "ECS.h"
#include "SharedDataRef.h"
#include <string>
#include <SDL3\SDL.h>
#include "StartState.h"
#include "Utility.h"

static bool mainLoopRegistered = false;

class Engine
{
public:
    
    SharedDataRef _data = std::make_shared<SharedData>();

    Engine() = default;
    ~Engine() = default;

    bool Initialize();
    void Update(float dt);
    void HandleInput(float dt);
    void Render(float dt);
    
    void run();


#ifdef __EMSCRIPTEN__
    bool InitializeWeb(const std::string& elementId);
#endif // __EMSCRIPTEN__

};