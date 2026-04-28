#pragma once
#include "SharedDataRef.h"
#include "State.h"

class depthState : public State
{
public:
    using State::State;
    void Init() override;
    void HandleInput() override {}
    void Update(float dt) override;
};