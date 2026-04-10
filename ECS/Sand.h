#pragma once
#include "SharedDataRef.h"
#include "State.h"

class Sand : public State
{
public:
    using State::State;
    void Init() override;
    void Update(float dt) override;

private:
};

