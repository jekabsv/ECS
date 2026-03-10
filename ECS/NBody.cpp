#include "NBody.h"
#include <cstdlib>
#include <cmath>

void NBody::Init()
{
    ecs.Tie(_data);

    const float W = (float)_data->GAME_WIDTH;
    const float H = (float)_data->GAME_HEIGHT * 0.5f;

    for (int i = 0; i < N; i++)
    {
        ECS::Entity e = ecs.Create();
        StarComponent star;
        star.x = (float)(rand() % (int)W);
        star.y = (float)(rand() % (int)H);
        star.vx = ((rand() % 400) - 200) * 0.3f;
        star.vy = ((rand() % 400) - 200) * 0.3f;
        star.mass = 80.0f + (float)(rand() % 120);
        ecs.Add<StarComponent>(e, star);
    }

    ecs.RegisterSystem<StarComponent>("applyGravity",
        [](ECS::Entity, ECS::ComponentContext ctx, float dt, SharedDataRef data)
        {
            auto& s = ctx.Get<StarComponent>();

            float ax = 0.0f, ay = 0.0f;
            ctx.Query<StarComponent>([&](ECS::Entity, StarComponent& other)
                {
                    float dx = other.x - s.x;
                    float dy = other.y - s.y;
                    float distSq = dx * dx + dy * dy + SOFTENING_SQ;
                    float dist = std::sqrt(distSq);
                    float a = G * other.mass / distSq;
                    ax += a * dx / dist;
                    ay += a * dy / dist;
                });

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
        },
        ECS::SystemGroup::Update);

    ecs.RegisterSystem<StarComponent>("renderStars",
        [](ECS::Entity, ECS::ComponentContext ctx, float, SharedDataRef data)
        {
            auto& s = ctx.Get<StarComponent>();
            Uint8 b = (Uint8)(160 + (int)((s.mass - 80) * 0.475f));
            SDL_SetRenderDrawColor(data->SDLrenderer, b, b, (Uint8)(b * 0.7f), 255);
            SDL_FRect rect = { s.x - 1.5f, s.y - 1.5f, 3.0f, 3.0f };
            SDL_RenderFillRect(data->SDLrenderer, &rect);
            SDL_SetRenderDrawColor(data->SDLrenderer, 0, 0, 0, 0);
        },
        ECS::SystemGroup::Render);
}