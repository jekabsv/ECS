#pragma once
#include "ECS.h"
#include <string>
#include <SDL3\SDL.h>
#include "RenderComponent.h"
#include "StateMachine.h"
#include "StartState.h"
#include <iostream>
#include "SharedDataRef.h"
#include "AssetManager.h"

static bool mainLoopRegistered = false;



class Engine
{
public:
    
    SharedDataRef _data = std::make_shared<SharedData>();

    Engine() = default;
    ~Engine() = default;

    bool Initialize();
    void Update(float deltaTime);
    void Render();


#ifdef __EMSCRIPTEN__
    bool InitializeWeb(const std::string& elementId);
#endif // __EMSCRIPTEN__

};