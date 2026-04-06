#pragma once
#include "SharedDataRef.h"
#include "State.h"

class NBody : public State
{
public:
    using State::State;
    void Init() override;

private:
    struct StarComponent
    {
        float mass = 1.0f;
    };

    static constexpr int   N = 1000;
    static constexpr float G = 1200.0f;
    static constexpr float SOFTENING_SQ = 300.0f;
    static constexpr float MAX_SPEED = 800.0f;
};