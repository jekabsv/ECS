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
    int TexX;
    int TexY;
    bool direction = 0;
    ecs::Entity e;
    ecs::Entity player;
    Uint64 start = SDL_GetTicks();
};



