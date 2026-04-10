#pragma once
#include "SharedDataRef.h"
#include "State.h"

class Boids : public State
{
public:
    using State::State;
    void Init() override;
    void Update(float dt) override;

private:
    struct BoidComponent
    {
        float angle = 0.0f;
    };

    static constexpr int   COUNT = 5000;
    static constexpr float PERCEPTION = 80.0f;
    static constexpr float SEPARATION_DIST = 20.0f;

    static constexpr float MIN_SPEED = 80.0f;
    static constexpr float MAX_SPEED = 220.0f;
    static constexpr float MAX_TURN_RATE = 2.5f;



    static constexpr float W_SEPARATION = 1.8f;
    static constexpr float W_ALIGNMENT = 1.0f;
    static constexpr float W_COHESION = 0.9f;
};