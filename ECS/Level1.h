#pragma once
#include "SharedDataRef.h"
#include "State.h"

class Level1 : public State
{
public:
    using State::State;
    void Init() override;
    void HandleInput() override {}
    void Update(float dt) override;

private:
    int TexX = 0;
    int TexY = 0;
    bool direction = 0;
    ECS::Entity e = 0;
    ECS::Entity playerEntity = 0;
    Uint64 start = SDL_GetTicks();
};
