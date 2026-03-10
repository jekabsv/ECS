#pragma once
#include "SharedDataRef.h"
#include "State.h"


struct Position { float x = 0, y = 0; };
struct Velocity { float vx = 0, vy = 0; };
struct Health { float hp = 100, maxHp = 100, regenRate = 5.0f; };
struct Lifetime { float remaining = 5.0f; };
struct Team { int id = 0; };
struct DrawColor { uint8_t r, g, b, a; };


class BenchmarkState : public State
{
public:
    BenchmarkState(SharedDataRef data) : _data(data) {}
    void Init()   override;
    void Update(float dt) override;
    void Render(float dt) override;

private:
    SharedDataRef _data;
    static constexpr int N = 100000;

    uint64_t _updateUs = 0;
    uint64_t _renderUs = 0;
    uint64_t _freq = 0;
    uint64_t _logTimer = 0;
};