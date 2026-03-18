#include "FusionBenchmark.h"
#include "logger.h"
#include <cstdlib>
#include <cmath>

// ---------------------------------------------------------
// Shared system lambdas
// ---------------------------------------------------------
// UpdateTransform: rotates the matrix slightly each frame so the data is
// actually written and the compiler cannot dead-strip the reads below.
static auto MakeUpdateTransform()
{
    return [](ECS::ArchetypeContext ctx, float dt, SharedDataRef)
        {
            auto transforms = ctx.Slice<Transform>();
            const float s = std::sin(dt * 0.1f);
            const float c = std::cos(dt * 0.1f);
            for (auto& t : transforms)
            {
                // Cheap in-place rotation in XZ plane � forces a full read+write of the matrix
                float m0 = t.matrix[0] * c - t.matrix[2] * s;
                float m2 = t.matrix[0] * s + t.matrix[2] * c;
                float m4 = t.matrix[4] * c - t.matrix[6] * s;
                float m6 = t.matrix[4] * s + t.matrix[6] * c;
                float m8 = t.matrix[8] * c - t.matrix[10] * s;
                float m10 = t.matrix[8] * s + t.matrix[10] * c;
                t.matrix[0] = m0;  t.matrix[2] = m2;
                t.matrix[4] = m4;  t.matrix[6] = m6;
                t.matrix[8] = m8;  t.matrix[10] = m10;
            }
        };
}

// FrustumCull: reads all 16 floats of the matrix to compute a simple
// projection test � forces the full Transform column into cache.
static auto MakeFrustumCull()
{
    return [](ECS::ArchetypeContext ctx, float, SharedDataRef)
        {
            auto transforms = ctx.Slice<Transform>();
            auto visible = ctx.Slice<Visible>();
            for (std::size_t i = 0; i < transforms.size(); i++)
            {
                const auto& m = transforms[i].matrix;
                float wx = m[3] * m[12] + m[7] * m[13] + m[11] * m[14] + m[15];
                visible[i].value = (wx > -1.0f) ? 1.0f : 0.0f;
            }
        };
}

// ShadowCull: similar read of the matrix � if Transform is still hot from
// FrustumCull (fused), this is nearly free. If evicted (unfused), reload.
static auto MakeShadowCull()
{
    return [](ECS::ArchetypeContext ctx, float, SharedDataRef)
        {
            auto transforms = ctx.Slice<Transform>();
            auto shadow = ctx.Slice<ShadowCaster>();
            for (std::size_t i = 0; i < transforms.size(); i++)
            {
                const auto& m = transforms[i].matrix;
                float wy = m[1] * m[12] + m[5] * m[13] + m[9] * m[14] + m[13];
                shadow[i].value = (wy > 0.0f) ? 1.0f : 0.0f;
            }
        };
}

// LODSelect: reads translation column of the matrix to compute distance.
static auto MakeLODSelect()
{
    return [](ECS::ArchetypeContext ctx, float, SharedDataRef)
        {
            auto transforms = ctx.Slice<Transform>();
            auto lod = ctx.Slice<LODLevel>();
            for (std::size_t i = 0; i < transforms.size(); i++)
            {
                const auto& m = transforms[i].matrix;
                float distSq = m[12] * m[12] + m[13] * m[13] + m[14] * m[14];
                lod[i].level = (distSq < 10000.0f) ? 0.0f
                    : (distSq < 40000.0f) ? 1.0f : 2.0f;
            }
        };
}

// ---------------------------------------------------------

void FusionBenchmark::InitWorld(ECS::World& world, bool fused)
{
    world.Tie(_data);

    for (int i = 0; i < N; i++)
    {
        ECS::Entity e = world.Create();

        Transform t;
        // Spread entities so LOD levels vary
        t.matrix[12] = (float)(rand() % 1000) - 500.0f;
        t.matrix[13] = (float)(rand() % 1000) - 500.0f;
        t.matrix[14] = (float)(rand() % 1000) - 500.0f;

        world.Add<Transform>(e, t);
        world.Add<Visible>(e, {});
        world.Add<ShadowCaster>(e, {});
        world.Add<LODLevel>(e, {});
    }

    // UpdateTransform writes Transform � must always be its own group regardless of fusing,
    // because all three culling systems RAW-depend on it.
    if (fused)
    {
        world.RegisterSystem<Transform>("updateTransform", MakeUpdateTransform())
            .Read<Transform>()
            .Write<Transform>();

        // All three read Transform, write disjoint outputs � no conflicts, they fuse.
        world.RegisterSystem<Transform, Visible>("frustumCull", MakeFrustumCull())
            .Read<Transform>()
            .Write<Visible>();

        world.RegisterSystem<Transform, ShadowCaster>("shadowCull", MakeShadowCull())
            .Read<Transform>()
            .Write<ShadowCaster>();

        world.RegisterSystem<Transform, LODLevel>("lodSelect", MakeLODSelect())
            .Read<Transform>()
            .Write<LODLevel>();
    }
    else
    {
        // No Read/Write declarations � every system conflicts with everything,
        // each gets its own group, four separate passes over all chunks.
        world.RegisterSystem<Transform>("updateTransform", MakeUpdateTransform());
        world.RegisterSystem<Transform, Visible>("frustumCull", MakeFrustumCull());
        world.RegisterSystem<Transform, ShadowCaster>("shadowCull", MakeShadowCull());
        world.RegisterSystem<Transform, LODLevel>("lodSelect", MakeLODSelect());
    }
}

void FusionBenchmark::Init()
{
    _freq = SDL_GetPerformanceFrequency();
    if (_freq == 0)
        _freq = 1;

    InitWorld(_fusedWorld, true);
    InitWorld(_unfusedWorld, false);
}

void FusionBenchmark::Update(float dt)
{
    if (_data->inputs.IsPressed("next"))
    {
        // Compute average for the world we just finished running
        uint64_t avg = (_sampleCount > 0) ? (_sampleSum / _sampleCount) : 0;
        if (_useFused)
            _fusedAvg = avg;
        else
            _unfusedAvg = avg;

        LOG_INFO(GlobalLogger(), "FusionBenchmark",
            std::string(_useFused ? "FUSED" : "UNFUSED") + " avg over "
            + std::to_string(_sampleCount) + " samples: " + std::to_string(avg) + "us"
            + "  |  switching to " + (_useFused ? "UNFUSED" : "FUSED"));

        _sampleSum = 0;
        _sampleCount = 0;
        _useFused = !_useFused;
    }

    ECS::World& active = _useFused ? _fusedWorld : _unfusedWorld;

    uint64_t start = SDL_GetPerformanceCounter();
    active.Run(ECS::SystemGroup::Update, dt);
    uint64_t us = (SDL_GetPerformanceCounter() - start) * 1000000 / _freq;

    _sampleSum += us;
    _sampleCount++;
}

void FusionBenchmark::Render(float dt)
{
    SDL_Renderer* r = _data->SDLrenderer;

    SDL_SetRenderDrawColor(r, 15, 15, 25, 255);

    uint64_t liveUs = (_sampleCount > 0) ? (_sampleSum / _sampleCount) : 0;

    uint64_t fusedDisplay = _useFused ? liveUs : _fusedAvg;
    uint64_t unfusedDisplay = !_useFused ? liveUs : _unfusedAvg;

    auto DrawBar = [&](float y, uint64_t us, uint8_t red, uint8_t green, uint8_t blue)
        {
            constexpr float MAX_US = 8000.0f;
            constexpr float BAR_MAX = 800.0f;
            float w = std::min((float)us / MAX_US, 1.0f) * BAR_MAX;
            SDL_SetRenderDrawColor(r, red, green, blue, 255);
            SDL_FRect bar = { 200.0f, y, w, 36.0f };
            SDL_RenderFillRect(r, &bar);
        };

    DrawBar(300.0f, fusedDisplay, 60, 200, 80);
    DrawBar(380.0f, unfusedDisplay, 200, 60, 60);

    // Outline the active bar
    SDL_SetRenderDrawColor(r, 255, 255, 255, 255);
    SDL_FRect outline = { 198.0f, (_useFused ? 298.0f : 378.0f), 804.0f, 40.0f };
    SDL_RenderRect(r, &outline);

    std::string fusedLabel = "FUSED   " + std::to_string(fusedDisplay) + " us";
    std::string unfusedLabel = "UNFUSED " + std::to_string(unfusedDisplay) + " us";

    if (_useFused)
        fusedLabel += "  (" + std::to_string(_sampleCount) + " samples)";
    else
        unfusedLabel += "  (" + std::to_string(_sampleCount) + " samples)";

    SDL_SetRenderDrawColor(r, 60, 200, 80, 255);
    SDL_RenderDebugText(r, 40.0f, 312.0f, fusedLabel.c_str());

    SDL_SetRenderDrawColor(r, 200, 60, 60, 255);
    SDL_RenderDebugText(r, 40.0f, 392.0f, unfusedLabel.c_str());

    SDL_SetRenderDrawColor(r, 200, 200, 200, 255);
    SDL_RenderDebugText(r, 40.0f, 460.0f, "SPACE � finalize current average and switch");
    SDL_RenderDebugText(r, 40.0f, 476.0f, ("N = " + std::to_string(N) + "  |  3 culling systems share Transform (64 bytes/entity)").c_str());
    SDL_RenderDebugText(r, 40.0f, 492.0f, "Fused: 2 chunk passes   Unfused: 4 chunk passes");
}