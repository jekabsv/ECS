#pragma once
#include "SharedDataRef.h"
#include "State.h"

struct StarComponent
{
    float x = 0.0f;
    float y = 0.0f;
    float vx = 0.0f;
    float vy = 0.0f;
    float mass = 1.0f;
};

class NBody : public State
{
public:
    NBody(SharedDataRef data) : _data(data) {}
    void Init() override;

private:
    SharedDataRef _data;

    static constexpr int   N = 1000;
    static constexpr float G = 1200.0f;
    static constexpr float SOFTENING_SQ = 300.0f;
    static constexpr float MAX_SPEED = 800.0f;
};