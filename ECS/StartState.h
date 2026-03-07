#pragma once
#include "StateMachine.h"
#include <iostream>
#include "SharedDataRef.h"

class StartState : public State
{
public:
    StartState(SharedDataRef data) : _data(data) {}

    void Init() override;
    void HandleInput() override {}
    void Update() override;
    void Render() override;

private:
    SharedDataRef _data;
    int TexX = 0;
    int TexY = 0;
    bool direction = 0;
    ecs::Entity e = 0;
    ecs::Entity player = 0;
    Uint64 start = SDL_GetTicks();
};



