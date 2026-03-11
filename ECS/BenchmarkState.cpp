#include "BenchmarkState.h"
#include "logger.h"
#include <cstdlib>
#include <cmath>



struct AllComponents
{
    float x, y;
    float vx, vy;
    float hp, maxHp, regenRate;
    float lifetime;
    int team;
    uint8_t r, g, b;
};



/*void BenchmarkState::Init()
{
    _freq = SDL_GetPerformanceFrequency();
    if (_freq == 0) 
        _freq = 1; 
    ecs.Tie(_data);

    const float W = (float)_data->GAME_WIDTH;
    const float H = (float)_data->GAME_HEIGHT;

    for (int i = 0; i < N; i++)
    {
        ECS::Entity e = ecs.Create();
        ecs.Add<AllComponents>(e, {
            (float)(rand() % (int)W), (float)(rand() % (int)H),
            ((rand() % 400) - 200) * 0.5f, ((rand() % 400) - 200) * 0.5f,
            100.0f, 100.0f, 5.0f,
            3.0f + (rand() % 5),
            rand() % 2,
            (uint8_t)(rand() % 255), (uint8_t)(rand() % 255), (uint8_t)(rand() % 255)
            });
    }

    ecs.RegisterSystem<AllComponents>("update",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
        {
            auto entities = ctx.Slice<AllComponents>();
            const float W = (float)data->GAME_WIDTH;
            const float H = (float)data->GAME_HEIGHT;
            for (auto& e : entities)
            {
                e.x += e.vx * dt;
                e.y += e.vy * dt;
                e.vx *= (1.0f - 0.5f * dt);
                e.vy *= (1.0f - 0.5f * dt);
                e.hp = std::min(e.maxHp, e.hp + e.regenRate * dt);
                e.lifetime -= dt;
                if (e.x < 0) { e.x = 0; e.vx = std::abs(e.vx); }
                if (e.x > W) { e.x = W; e.vx = -std::abs(e.vx); }
                if (e.y < 0) { e.y = 0; e.vy = std::abs(e.vy); }
                if (e.y > H) { e.y = H; e.vy = -std::abs(e.vy); }
            }
        }, ECS::SystemGroup::Update);

    ecs.RegisterSystem<AllComponents>("render",
        [](ECS::ArchetypeContext ctx, float, SharedDataRef data)
        {
            auto entities = ctx.Slice<AllComponents>();
            for (auto& e : entities)
            {
                float hpRatio = e.hp / e.maxHp;
                SDL_SetRenderDrawColor(data->SDLrenderer,
                    (uint8_t)(e.r * hpRatio),
                    (uint8_t)(e.g * hpRatio),
                    (uint8_t)(e.b * hpRatio), 255);
                SDL_FRect rect = { e.x - 2, e.y - 2, 4, 4 };
                SDL_RenderFillRect(data->SDLrenderer, &rect);
            }
        }, ECS::SystemGroup::Render);

}*/



void BenchmarkState::Init()
{
    _freq = SDL_GetPerformanceFrequency();
    if (_freq == 0)
        _freq = 1;
    ecs.Tie(_data);

    const float W = (float)_data->GAME_WIDTH;
    const float H = (float)_data->GAME_HEIGHT;

    for (int i = 0; i < N; i++)
    {
        ECS::Entity e = ecs.Create();

        ecs.Add<Position>(e, { (float)(rand() % (int)W), (float)(rand() % (int)H) });
        ecs.Add<Velocity>(e, { ((rand() % 400) - 200) * 0.5f, ((rand() % 400) - 200) * 0.5f });
        ecs.Add<Health>(e, { 100.0f, 100.0f, 5.0f });
        ecs.Add<Lifetime>(e, { 3.0f + (rand() % 5) });
        ecs.Add<Team>(e, { rand() % 2 });
        ecs.Add<DrawColor>(e, {
            (uint8_t)(rand() % 255),
            (uint8_t)(rand() % 255),
            (uint8_t)(rand() % 255),
            255 });
    }

    // Group 1: integrate only.
    // friction cannot join — it writes Velocity which integrate reads (WAR).
    ecs.RegisterSystem<Position, Velocity>("integrate",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
        {
            auto pos = ctx.Slice<Position>();
            auto vel = ctx.Slice<Velocity>();
            for (std::size_t i = 0; i < pos.size(); i++)
            {
                pos[i].x += vel[i].vx * dt;
                pos[i].y += vel[i].vy * dt;
            }
        }, ECS::SystemGroup::Update)
        .Read<Velocity>()
        .Write<Position>();

    // Group 2: friction + healthRegen + lifetime — no conflicts between the three.
    // boundsCheck cannot join — it writes Velocity which friction reads (WAR).
    ecs.RegisterSystem<Velocity>("friction",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef)
        {
            auto vel = ctx.Slice<Velocity>();
            for (auto& v : vel)
            {
                v.vx *= (1.0f - 0.5f * dt);
                v.vy *= (1.0f - 0.5f * dt);
            }
        }, ECS::SystemGroup::Update)
        .Read<Velocity>()
        .Write<Velocity>();

    ecs.RegisterSystem<Health>("healthRegen",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef)
        {
            auto health = ctx.Slice<Health>();
            for (auto& h : health)
                h.hp = std::min(h.maxHp, h.hp + h.regenRate * dt);
        }, ECS::SystemGroup::Update)
        .Read<Health>()
        .Write<Health>();

    ecs.RegisterSystem<Lifetime>("lifetime",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef)
        {
            auto lifetimes = ctx.Slice<Lifetime>();
            for (auto& l : lifetimes)
                l.remaining -= dt;
        }, ECS::SystemGroup::Update)
        .Write<Lifetime>();

    // Group 3: boundsCheck only.
    // renderEntities cannot join — it reads Position which boundsCheck writes (RAW).
    ecs.RegisterSystem<Position, Velocity>("boundsCheck",
        [](ECS::ArchetypeContext ctx, float, SharedDataRef data)
        {
            auto pos = ctx.Slice<Position>();
            auto vel = ctx.Slice<Velocity>();
            const float W = (float)data->GAME_WIDTH;
            const float H = (float)data->GAME_HEIGHT;
            for (std::size_t i = 0; i < pos.size(); i++)
            {
                if (pos[i].x < 0) { pos[i].x = 0; vel[i].vx = std::abs(vel[i].vx); }
                if (pos[i].x > W) { pos[i].x = W; vel[i].vx = -std::abs(vel[i].vx); }
                if (pos[i].y < 0) { pos[i].y = 0; vel[i].vy = std::abs(vel[i].vy); }
                if (pos[i].y > H) { pos[i].y = H; vel[i].vy = -std::abs(vel[i].vy); }
            }
        }, ECS::SystemGroup::Update)
        .Read<Position>()
        .Write<Position>()
        .Read<Velocity>()
        .Write<Velocity>();

    // Group 4: renderEntities only (render group, separate from update).
    ecs.RegisterSystem<Position, DrawColor, Health>("renderEntities",
        [](ECS::ArchetypeContext ctx, float, SharedDataRef data)
        {
            auto pos = ctx.Slice<Position>();
            auto colors = ctx.Slice<DrawColor>();
            auto health = ctx.Slice<Health>();
            for (std::size_t i = 0; i < pos.size(); i++)
            {
                float hpRatio = health[i].hp / health[i].maxHp;
                SDL_SetRenderDrawColor(data->SDLrenderer,
                    (uint8_t)(colors[i].r * hpRatio),
                    (uint8_t)(colors[i].g * hpRatio),
                    (uint8_t)(colors[i].b * hpRatio), 255);
                SDL_FRect rect = { pos[i].x - 2, pos[i].y - 2, 4, 4 };
                SDL_RenderFillRect(data->SDLrenderer, &rect);
            }
        }, ECS::SystemGroup::Render)
        .Read<Position>()
        .Read<DrawColor>()
        .Read<Health>();

    _freq = SDL_GetPerformanceFrequency();
}

void BenchmarkState::Update(float dt)
{
    uint64_t start = SDL_GetPerformanceCounter();
    ecs.Run(ECS::SystemGroup::Update, dt);
    _updateUs = (SDL_GetPerformanceCounter() - start) * 1000000 / _freq;
}

void BenchmarkState::Render(float dt)
{
    uint64_t start = SDL_GetPerformanceCounter();
    ecs.Run(ECS::SystemGroup::Render, dt);
    _renderUs = (SDL_GetPerformanceCounter() - start) * 1000000 / _freq;

    _logTimer += _updateUs + _renderUs;
    if (_logTimer >= 1000000)
    {
        LOG_INFO(GlobalLogger(), "Benchmark",
            "Update: " + std::to_string(_updateUs) + "us  "
            "Render: " + std::to_string(_renderUs) + "us  "
            "N=" + std::to_string(N));
        _logTimer = 0;
    }
}