#pragma once
#include "SharedDataRef.h"
#include "State.h"

// 64-byte component � large enough that repeated chunk loads are measurable
struct Transform
{
    float matrix[16] = {
        1,0,0,0,
        0,1,0,0,
        0,0,1,0,
        0,0,0,1
    };
};

// Small output components written by each culling system
struct Visible { float value = 1.0f; };
struct ShadowCaster { float value = 1.0f; };
struct LODLevel { float level = 0.0f; };

class FusionBenchmark : public State
{
public:
    FusionBenchmark(SharedDataRef data) : _data(data) {}

    void Init()             override;
    void Update(float dt)   override;
    void Render(float dt)   override;

private:
    void InitWorld(ECS::World& world, bool fused);

    SharedDataRef _data;

    // Two worlds � identical entities, identical system logic, different access declarations
    ECS::World _fusedWorld;
    ECS::World _unfusedWorld;

    bool     _useFused = true;
    uint64_t _freq = 1;
    uint64_t _fusedAvg = 0;
    uint64_t _unfusedAvg = 0;
    uint64_t _sampleSum = 0;
    uint64_t _sampleCount = 0;

    static constexpr int N = 50000;
};