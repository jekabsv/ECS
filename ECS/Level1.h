#pragma once
#include "SharedDataRef.h"
#include "State.h"

class Level1 : public State
{
public:
    Level1(SharedDataRef data) : _data(data) {}

    void Init() override;
    void HandleInput() override {}
    void Update(float dt) override;

private:
    SharedDataRef _data;
    int TexX = 0;
    int TexY = 0;
    bool direction = 0;
    ECS::Entity e = 0;
    ECS::Entity playerEntity = 0;
    Uint64 start = SDL_GetTicks();
};
