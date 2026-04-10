#pragma once
#include "State.h"
#include "SharedDataRef.h"
#include <vector>

class SPH : public State
{
public:
    using State::State;
    void Init()         override;
    void Update(float dt) override;

    // ── Tunables ──────────────────────────────────────────────────────────────
    static constexpr int   N = 200;
    static constexpr float H = 20.0f;   // smoothing radius (px)
    static constexpr float REST_DENSITY = 30.0f;   // rest density
    static constexpr float GAS_CONSTANT = 200.0f;  // stiffness — keep low for stability
    static constexpr float VISCOSITY = 200.0f;  // µ
    static constexpr float GRAVITY = 500.0f;  // px/s²
    static constexpr float PARTICLE_MASS = 1.0f;
    static constexpr float PARTICLE_DRAW = 4.0f;    // render half-size px
    static constexpr float DAMPING = 0.3f;    // velocity kept on wall bounce
    static constexpr float MAX_VEL = 400.0f;  // hard velocity clamp px/s

    // ── Kernel coefficients ───────────────────────────────────────────────────
    static constexpr float PI = 3.14159265358979f;
    static constexpr float POLY6_C = 315.0f / (64.0f * PI);
    static constexpr float SPIKY_C = -45.0f / PI;
    static constexpr float VISC_C = 45.0f / PI;

    // ── Per-particle component ────────────────────────────────────────────────
    struct SPHParticle
    {
        float density = 1.0f;
        float pressure = 0.0f;
    };

    // ── Neighbour cache entry — built once per frame, reused by two systems ──
    struct Neighbour
    {
        ECS::Entity entity;  // handle — used by sph_forces to TryGet current-frame SPHParticle
        float dx, dy;   // from i to j
        float r;        // distance
        float r2;       // distance squared
        // velocity snapshot — fine to be one frame stale for viscosity
        float vx_j, vy_j;
        // kept for potential future use; sph_forces now reads these fresh via entity
        float density_j;
        float pressure_j;
    };

    // neighbour_cache[i] = list of neighbours for particle i (in chunk order)
    // Rebuilt every frame by sph_build, read by sph_density + sph_forces.
    // Stored on the State so lambda captures just take a pointer.
    std::vector<std::vector<Neighbour>> neighbour_cache;
};