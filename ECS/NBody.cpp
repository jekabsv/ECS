#include "NBody.h"
#include <cstdlib>
#include <cmath>
#include "RenderComponent.h"



void NBody::Init()
{
    ecs.Tie(_data);

    const float W = (float)_data->GAME_WIDTH;
    const float H = (float)_data->GAME_HEIGHT * 0.5f;

    for (int i = 0; i < N; i++)
    {
        ECS::Entity e = ecs.Create();
        StarComponent star;
        //Position pos;
        //Velocity vel;
        star.x = (float)(rand() % (int)W);
        star.y = (float)(rand() % (int)H);
        star.vx = ((rand() % 400) - 200) * 0.3f;
        star.vy = ((rand() % 400) - 200) * 0.3f;
        star.mass = 80.0f + (float)(rand() % 120);
        ecs.Add<StarComponent>(e, star);
       /* ecs.Add<Position>(e, pos);
        ecs.Add<Velocity>(e, vel);*/
    }

    ecs.RegisterSystem<StarComponent>("applyGravity",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
        {
            auto stars = ctx.Slice<StarComponent>();

            for (auto& s : stars)
            {
                float ax = 0.0f, ay = 0.0f;

                for (auto& otherCtx : ctx.View<StarComponent>())
                {
                    auto others = otherCtx.Slice<StarComponent>();
                    for (auto& other : others)
                    {
                        float dx = other.x - s.x;
                        float dy = other.y - s.y;
                        float distSq = dx * dx + dy * dy + SOFTENING_SQ;
                        float dist = std::sqrt(distSq);
                        float a = G * other.mass / distSq;
                        ax += a * dx / dist;
                        ay += a * dy / dist;
                    }
                }

                s.vx += ax * dt;
                s.vy += ay * dt;

                float speedSq = s.vx * s.vx + s.vy * s.vy;
                if (speedSq > MAX_SPEED * MAX_SPEED)
                {
                    float scale = MAX_SPEED / std::sqrt(speedSq);
                    s.vx *= scale;
                    s.vy *= scale;
                }

                s.x += s.vx * dt;
                s.y += s.vy * dt;

                const float W = (float)data->GAME_WIDTH;
                const float H = (float)data->GAME_HEIGHT * 0.5f;
                if (s.x < 0.0f) { s.x = 0.0f; s.vx = std::abs(s.vx); }
                if (s.x > W) { s.x = W;     s.vx = -std::abs(s.vx); }
                if (s.y < 0.0f) { s.y = 0.0f;  s.vy = std::abs(s.vy); }
                if (s.y > H) { s.y = H;      s.vy = -std::abs(s.vy); }
            }
        },
        ECS::SystemGroup::Update);

    ecs.RegisterSystem<StarComponent>("renderStars",
        [](ECS::ArchetypeContext ctx, float, SharedDataRef data)
        {
            auto stars = ctx.Slice<StarComponent>();

            for (auto& s : stars)
            {
                Uint8 b = (Uint8)(160 + (int)((s.mass - 80) * 0.475f));
                SDL_SetRenderDrawColor(data->SDLrenderer, b, b, (Uint8)(b * 0.7f), 255);
                SDL_FRect rect = { s.x - 1.5f, s.y - 1.5f, 3.0f, 3.0f };
                SDL_RenderFillRect(data->SDLrenderer, &rect);
                SDL_SetRenderDrawColor(data->SDLrenderer, 0, 0, 0, 0);
            }
        },
        ECS::SystemGroup::Render);
    /*


    ecs.RegisterSystem<StarComponent, Position, Velocity>("applyGravity",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
        {
            auto positions = ctx.Slice<Position>();
            auto velocities = ctx.Slice<Velocity>();
            auto masses = ctx.Slice<StarComponent>();

            for (std::size_t i = 0; i < positions.size(); i++)
            {
                float ax = 0.0f, ay = 0.0f;

                for (auto& otherCtx : ctx.View<StarComponent, Position>())
                {
                    auto othersPos = otherCtx.Slice<Position>();
                    auto othersMass = otherCtx.Slice<StarComponent>();
                    for (std::size_t j = 0; j < othersPos.size(); j++)
                    {
                        float dx = othersPos[j].x - positions[i].x;
                        float dy = othersPos[j].y - positions[i].y;
                        float distSq = dx * dx + dy * dy + SOFTENING_SQ;
                        float dist = std::sqrt(distSq);
                        float a = G * othersMass[j].mass / distSq;
                        ax += a * dx / dist;
                        ay += a * dy / dist;
                    }
                }

                velocities[i].vx += ax * dt;
                velocities[i].vy += ay * dt;

                float speedSq = velocities[i].vx * velocities[i].vx + velocities[i].vy * velocities[i].vy;
                if (speedSq > MAX_SPEED * MAX_SPEED)
                {
                    float scale = MAX_SPEED / std::sqrt(speedSq);
                    velocities[i].vx *= scale;
                    velocities[i].vy *= scale;
                }
            }
        },
        ECS::SystemGroup::Update);

    ecs.RegisterSystem <StarComponent, Velocity>("applyGravity",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
        {
            auto starsPos = ctx.Slice<Position>();
            auto starsVel = ctx.Slice<Velocity>();

            for (int i = 0;i < starsPos.size();i++)
            {
                starsPos[i].x += starsVel[i].vx * dt;
                starsPos[i].y += starsVel[i].vy * dt;

                const float W = (float)data->GAME_WIDTH;
                const float H = (float)data->GAME_HEIGHT * 0.5f;
                if (starsPos[i].x < 0.0f) { starsPos[i].x = 0.0f; starsVel[i].vx = std::abs(starsVel[i].vx); }
                if (starsPos[i].x > W) { starsPos[i].x = W;     starsVel[i].vx = -std::abs(starsVel[i].vx); }
                if (starsPos[i].y < 0.0f) { starsPos[i].y = 0.0f;  starsVel[i].vy = std::abs(starsVel[i].vy); }
                if (starsPos[i].y > H) { starsPos[i].y = H;      starsVel[i].vy = -std::abs(starsVel[i].vy); }
            }
        },
        ECS::SystemGroup::Update);

    ecs.RegisterSystem<StarComponent, Position>("renderStars",
        [](ECS::ArchetypeContext ctx, float, SharedDataRef data)
        {
            auto starsPos = ctx.Slice<Position>();
            auto starsMass = ctx.Slice<StarComponent>();

            for (int i = 0; i < starsPos.size(); i++)
            {
                Uint8 b = (Uint8)(160 + (int)((starsMass[i].mass - 80) * 0.475f));
                SDL_SetRenderDrawColor(data->SDLrenderer, b, b, (Uint8)(b * 0.7f), 255);
                SDL_FRect rect = { starsPos[i].x - 1.5f, starsPos[i].y - 1.5f, 3.0f, 3.0f};
                SDL_RenderFillRect(data->SDLrenderer, &rect);
                SDL_SetRenderDrawColor(data->SDLrenderer, 0, 0, 0, 0);
            }
        },
        ECS::SystemGroup::Render);*/
}