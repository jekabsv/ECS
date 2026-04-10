#include "SPH.h"
#include "Transform.h"
#include "RigidBody.h"
#include "BoxCollider.h"
#include "Level1.h"
#include <cmath>
#include <cstdlib>
#include <algorithm>
#include <SDL3/SDL.h>

// ─────────────────────────────────────────────────────────────────────────────
//  Inline kernels — everything pre-divided by the appropriate power of H
//  so we avoid recomputing it inside inner loops.
// ─────────────────────────────────────────────────────────────────────────────
namespace SPHKernel
{
    // Poly6 density kernel:  W = POLY6_C / h^9 * (h²-r²)^3
    inline float Poly6(float r2, float h2, float inv_h9)
    {
        if (r2 >= h2) return 0.0f;
        float d = h2 - r2;
        return SPH::POLY6_C * inv_h9 * (d * d * d);
    }

    // Spiky gradient magnitude: |∇W| = |SPIKY_C| / h^6 * (h-r)^2
    // Caller multiplies by (dx/r, dy/r) for direction.
    // Returns a negative value — caller gets a repulsive force when used as
    // pressure gradient.
    inline float SpikyGrad(float r, float inv_h6)
    {
        if (r <= 0.0f || r >= SPH::H) return 0.0f;
        float d = SPH::H - r;
        return SPH::SPIKY_C * inv_h6 * (d * d);
    }

    // Viscosity laplacian: ∇²W = VISC_C / h^6 * (h-r)
    inline float ViscLap(float r, float inv_h6)
    {
        if (r <= 0.0f || r >= SPH::H) return 0.0f;
        return SPH::VISC_C * inv_h6 * (SPH::H - r);
    }
}

// ─────────────────────────────────────────────────────────────────────────────
void SPH::Init()
{
    const float W = (float)_data->GAME_WIDTH;
    const float H_screen = (float)_data->GAME_HEIGHT;

    // Pre-compute H powers once, capture by value in lambdas
    const float h2 = H * H;
    const float h6 = h2 * h2 * h2;
    const float h9 = h6 * h2 * H;
    const float inv_h6 = 1.0f / h6;
    const float inv_h9 = 1.0f / h9;

    // Enable physics movement — it will integrate vx/vy → position each frame.
    // Collision detection off: SPH handles its own boundary response.
    _data->physics.EnableMovement();

    // ── Spawn: grid block in the top-left quarter (the "dam") ────────────────
    const float spacing = H * 0.5f;
    const int   cols = (int)std::ceil(std::sqrt((float)N * 0.55f));
    const int   rows = (int)std::ceil((float)N / cols);
    const float startX = 60.0f;
    const float startY = 60.0f;

    neighbour_cache.resize(N);

    int spawned = 0;
    for (int r = 0; r < rows && spawned < N; r++)
    {
        for (int c = 0; c < cols && spawned < N; c++)
        {
            ECS::Entity e = ecs.Create();

            TransformComponent tr;
            // slight stagger on odd rows to avoid a perfect grid (reduces initial pressure spikes)
            tr.position.x = startX + c * spacing + (r & 1) * spacing * 0.4f;
            tr.position.y = startY + r * spacing;
            tr.scale = { 1.0f, 1.0f };

            RigidBody rb;
            rb.isStatic = false;
            rb.drag = 0.0f;   // SPH handles damping at boundaries
            rb.vx = 0.0f;
            rb.vy = 0.0f;

            SPHParticle sp;
            sp.density = REST_DENSITY;
            sp.pressure = 0.0f;

            ecs.Add<TransformComponent>(e, tr);
            ecs.Add<RigidBody>(e, rb);
            ecs.Add<SPHParticle>(e, sp);

            spawned++;
        }
    }

    // ── Capture pointer to neighbour cache (lives on State, outlives lambdas) ─
    auto* cache = &neighbour_cache;

    // ─────────────────────────────────────────────────────────────────────────
    //  System 1 — sph_build
    //  Clears spatial index, inserts all particles, then queries and fills
    //  the neighbour cache. Density and pressure of neighbours are NOT yet
    //  valid this frame, so we only store position/velocity data here;
    //  density_j / pressure_j are filled by sph_density below.
    // ─────────────────────────────────────────────────────────────────────────
    ecs.RegisterSystem<SPHParticle, TransformComponent, RigidBody>(
        "sph_build",
        [cache, h2, inv_h9](ECS::ArchetypeContext ctx, float, SharedDataRef data)
        {
            data->spatialIndex.Clear();

            auto entities = ctx.Slice<ECS::Entity>();
            auto positions = ctx.Slice<TransformComponent>();
            auto rbs = ctx.Slice<RigidBody>();
            auto particles = ctx.Slice<SPHParticle>();

            const std::size_t count = entities.size();

            // Grow cache if needed (first frame or N changed)
            if (cache->size() < count)
                cache->resize(count);

            // Insert all into spatial index
            for (std::size_t i = 0; i < count; i++)
            {
                float px = positions[i].position.x;
                float py = positions[i].position.y;
                data->spatialIndex.InsertRectangle(
                    entities[i],
                    px - SPH::H, py - SPH::H,
                    SPH::H * 2.0f, SPH::H * 2.0f);
            }

            // Build flat per-particle neighbour lists.
            // We store a snapshot of each neighbour's position and velocity so
            // that sph_density and sph_forces never touch TryGet at all.
            std::vector<ECS::Entity> found;
            found.reserve(64);

            ECS::World* world = data->physics.GetWorld();

            for (std::size_t i = 0; i < count; i++)
            {
                float px = positions[i].position.x;
                float py = positions[i].position.y;

                found.clear();
                data->spatialIndex.QueryRectangle(
                    px - SPH::H, py - SPH::H,
                    SPH::H * 2.0f, SPH::H * 2.0f,
                    found);

                auto& nb_list = (*cache)[i];
                nb_list.clear();

                for (ECS::Entity nb : found)
                {
                    if (nb == entities[i]) continue;

                    TransformComponent* nbTr = world->TryGet<TransformComponent>(nb);
                    RigidBody* nbRb = world->TryGet<RigidBody>(nb);
                    SPHParticle* nbSph = world->TryGet<SPHParticle>(nb);
                    if (!nbTr || !nbRb || !nbSph) continue;

                    float dx = nbTr->position.x - px;
                    float dy = nbTr->position.y - py;
                    float r2 = dx * dx + dy * dy;
                    if (r2 >= SPH::H * SPH::H) continue;  // AABB false positive

                    float r = std::sqrt(r2);

                    SPH::Neighbour nb_entry;
                    nb_entry.entity = nb;
                    nb_entry.dx = dx;
                    nb_entry.dy = dy;
                    nb_entry.r = r;
                    nb_entry.r2 = r2;
                    nb_entry.vx_j = nbRb->vx;
                    nb_entry.vy_j = nbRb->vy;
                    // density_j/pressure_j stale here — sph_forces reads fresh via entity
                    nb_entry.density_j = nbSph->density;
                    nb_entry.pressure_j = nbSph->pressure;

                    nb_list.push_back(nb_entry);
                }
            }
        },
        ECS::SystemGroup::Update);

    // ─────────────────────────────────────────────────────────────────────────
    //  System 2 — sph_density
    //  Reads neighbour cache, accumulates Poly6 density, derives pressure.
    //  Writes back into SPHParticle — no TryGet, no spatial queries.
    // ─────────────────────────────────────────────────────────────────────────
    ecs.RegisterSystem<SPHParticle, TransformComponent, RigidBody>(
        "sph_density",
        [cache, h2, inv_h9](ECS::ArchetypeContext ctx, float, SharedDataRef)
        {
            auto particles = ctx.Slice<SPHParticle>();
            const std::size_t count = particles.size();

            for (std::size_t i = 0; i < count; i++)
            {
                // Self-contribution
                float density = SPH::PARTICLE_MASS * SPHKernel::Poly6(0.0f, h2, inv_h9);

                for (const auto& nb : (*cache)[i])
                    density += SPH::PARTICLE_MASS * SPHKernel::Poly6(nb.r2, h2, inv_h9);

                particles[i].density = std::max(density, 0.001f);  // never zero

                // Tait equation of state — clamp so pressure can't go wildly negative
                float p = SPH::GAS_CONSTANT * (particles[i].density - SPH::REST_DENSITY);
                particles[i].pressure = std::max(p, 0.0f);  // no tension
            }
        },
        ECS::SystemGroup::Update);

    // ─────────────────────────────────────────────────────────────────────────
    //  System 3 — sph_forces
    //  Symmetric pressure gradient (Müller 2003) + viscosity + gravity.
    //  Reads neighbour cache; cache has last-frame density_j / pressure_j
    //  which is one frame stale but stable and standard practice for SPH.
    //  Writes result directly into RigidBody.vx/vy.
    //  PhysicsSystem physicsMove then integrates those into position.
    // ─────────────────────────────────────────────────────────────────────────
    ecs.RegisterSystem<SPHParticle, TransformComponent, RigidBody>(
        "sph_forces",
        [cache, inv_h6](ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
        {
            auto particles = ctx.Slice<SPHParticle>();
            auto rbs = ctx.Slice<RigidBody>();
            const std::size_t count = particles.size();
            ECS::World* world = data->physics.GetWorld();

            for (std::size_t i = 0; i < count; i++)
            {
                if (rbs[i].isStatic) continue;

                float di = particles[i].density;   // guaranteed > 0 from sph_density
                float pi = particles[i].pressure;

                float fx = 0.0f, fy = 0.0f;  // pressure
                float vfx = 0.0f, vfy = 0.0f; // viscosity

                for (const auto& nb : (*cache)[i])
                {
                    // Read current-frame density/pressure — sph_density has already
                    // written these so they are not stale. Only two TryGets per
                    // neighbour, only in this pass.
                    SPHParticle* nbSph = world->TryGet<SPHParticle>(nb.entity);
                    if (!nbSph) continue;
                    float dj = nbSph->density;
                    float pj = nbSph->pressure;
                    if (dj < 0.001f) continue;

                    float r = nb.r;

                    // ── Pressure (symmetric form) ──
                    float grad_mag = SPHKernel::SpikyGrad(r, inv_h6);
                    // grad_mag is negative (SPIKY_C < 0), so this pushes apart
                    float pterm = SPH::PARTICLE_MASS
                        * (pi / (di * di) + pj / (dj * dj))
                        * grad_mag;

                    // direction: from j toward i (repulsive)
                    float inv_r = 1.0f / r;
                    fx += pterm * (-nb.dx * inv_r);
                    fy += pterm * (-nb.dy * inv_r);

                    // ── Viscosity ──
                    float lap = SPHKernel::ViscLap(r, inv_h6);
                    float visc_coeff = SPH::VISCOSITY * SPH::PARTICLE_MASS / dj * lap;
                    vfx += visc_coeff * (nb.vx_j - rbs[i].vx);
                    vfy += visc_coeff * (nb.vy_j - rbs[i].vy);
                }

                // a = F / density,  then Euler integrate into velocity
                float ax = (fx + vfx) / di;
                float ay = (fy + vfy) / di + SPH::GRAVITY;

                rbs[i].vx += ax * dt;
                rbs[i].vy += ay * dt;

                // Hard velocity clamp — prevents instability from blowing up
                float speed2 = rbs[i].vx * rbs[i].vx + rbs[i].vy * rbs[i].vy;
                if (speed2 > SPH::MAX_VEL * SPH::MAX_VEL)
                {
                    float scale = SPH::MAX_VEL / std::sqrt(speed2);
                    rbs[i].vx *= scale;
                    rbs[i].vy *= scale;
                }
            }
        },
        ECS::SystemGroup::Update);

    // ─────────────────────────────────────────────────────────────────────────
    //  System 4 — sph_bounds
    //  Reflect velocity and push position back inside screen boundaries.
    //  Runs after forces, before physicsMove integrates position.
    // ─────────────────────────────────────────────────────────────────────────
    ecs.RegisterSystem<SPHParticle, TransformComponent, RigidBody>(
        "sph_bounds",
        [W, H_screen](ECS::ArchetypeContext ctx, float, SharedDataRef)
        {
            auto positions = ctx.Slice<TransformComponent>();
            auto rbs = ctx.Slice<RigidBody>();
            const float R = SPH::PARTICLE_DRAW;

            for (std::size_t i = 0; i < positions.size(); i++)
            {
                float& x = positions[i].position.x;
                float& y = positions[i].position.y;

                if (x < R) { x = R;         rbs[i].vx = std::abs(rbs[i].vx) * SPH::DAMPING; }
                else if (x > W - R) { x = W - R;     rbs[i].vx = -std::abs(rbs[i].vx) * SPH::DAMPING; }

                if (y < R) { y = R;         rbs[i].vy = std::abs(rbs[i].vy) * SPH::DAMPING; }
                else if (y > H_screen - R) { y = H_screen - R; rbs[i].vy = -std::abs(rbs[i].vy) * SPH::DAMPING; }
            }
        },
        ECS::SystemGroup::Update);

    // ─────────────────────────────────────────────────────────────────────────
    //  System 5 — sph_render
    //  Colour by speed: deep blue (still) → cyan → white (fast).
    // ─────────────────────────────────────────────────────────────────────────
    ecs.RegisterSystem<SPHParticle, TransformComponent, RigidBody>(
        "sph_render",
        [](ECS::ArchetypeContext ctx, float, SharedDataRef data)
        {
            auto positions = ctx.Slice<TransformComponent>();
            auto rbs = ctx.Slice<RigidBody>();
            const float R = SPH::PARTICLE_DRAW;

            for (std::size_t i = 0; i < positions.size(); i++)
            {
                float speed = std::sqrt(rbs[i].vx * rbs[i].vx + rbs[i].vy * rbs[i].vy);
                float t = std::min(speed / SPH::MAX_VEL, 1.0f);

                Uint8 rv = (Uint8)(t * 180);
                Uint8 gv = (Uint8)(t * 220);
                Uint8 bv = 255;

                SDL_SetRenderDrawColor(data->SDLrenderer, rv, gv, bv, 230);

                SDL_FRect rect = {
                    positions[i].position.x - R,
                    positions[i].position.y - R,
                    R * 2.0f, R * 2.0f
                };
                SDL_RenderFillRect(data->SDLrenderer, &rect);
            }

            SDL_SetRenderDrawColor(data->SDLrenderer, 0, 0, 0, 255);
        },
        ECS::SystemGroup::Render);
}

// ─────────────────────────────────────────────────────────────────────────────
void SPH::Update(float)
{
    if (_data->inputs.GetActionState("next") == InputSystem::Pressed)
        _data->state.AddState(StateRef(new Level1(_data)), 1);
}