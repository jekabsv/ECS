#include "NBody.h"
#include <cstdlib>
#include <cmath>
#include "RenderComponent.h"
#include "Transform.h"


void NBody::Init()
{
    _data->physics.EnableMovement();
    const float W = (float)_data->GAME_WIDTH;
    const float H = (float)_data->GAME_HEIGHT * 0.5f;

    for (int i = 0; i < N; i++)
    {
        ECS::Entity e = ecs.Create();

        RigidBody rb;
        StarComponent star;
        TransformComponent tr;

        rb.drag = 0.0f;
        rb.isStatic = false;
        rb.vx = ((rand() % 400) - 200) * 0.3f;
        rb.vy = ((rand() % 400) - 200) * 0.3f;

        tr.position.x = (float)(rand() % (int)W);
        tr.position.y = (float)(rand() % (int)H);
        
        star.mass = 80.0f + (float)(rand() % 120);
        
        ecs.Add<StarComponent>(e, star);
        ecs.Add<TransformComponent>(e, tr);
        ecs.Add<RigidBody>(e, rb);
    }

    ecs.RegisterSystem<StarComponent, TransformComponent, RigidBody>("applyGravity",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
        {
            auto transforms = ctx.Slice<TransformComponent>();
            auto rigidBodies = ctx.Slice<RigidBody>();
            auto masses = ctx.Slice<StarComponent>();

            for (std::size_t i = 0; i < transforms.size(); i++)
            {
                float ax = 0.0f, ay = 0.0f;

                for (auto& otherCtx : ctx.View<StarComponent, TransformComponent>())
                {
                    auto othersTransform = otherCtx.Slice<TransformComponent>();
                    auto othersMass = otherCtx.Slice<StarComponent>();
                    for (std::size_t j = 0; j < othersTransform.size(); j++)
                    {
                        float dx = othersTransform[j].position.x - transforms[i].position.x;
                        float dy = othersTransform[j].position.y - transforms[i].position.y;

                        float distSq = dx * dx + dy * dy + SOFTENING_SQ;
                        float dist = std::sqrt(distSq);
                        float a = G * othersMass[j].mass / distSq;
                        ax += a * dx / dist;
                        ay += a * dy / dist;
                    }
                }

                rigidBodies[i].vx += ax * dt;
                rigidBodies[i].vy += ay * dt;

                float speedSq = rigidBodies[i].vx * rigidBodies[i].vx + rigidBodies[i].vy * rigidBodies[i].vy;
                if (speedSq > MAX_SPEED * MAX_SPEED)
                {
                    float scale = MAX_SPEED / std::sqrt(speedSq);
                    rigidBodies[i].vx *= scale;
                    rigidBodies[i].vy *= scale;
                }
            }
        },
        ECS::SystemGroup::Update);

    ecs.RegisterSystem<StarComponent, TransformComponent>("renderStars",
        [](ECS::ArchetypeContext ctx, float, SharedDataRef data)
        {
            auto starsTransform = ctx.Slice<TransformComponent>();
            auto starsMass = ctx.Slice<StarComponent>();

            for (int i = 0; i < starsTransform.size(); i++)
            {
                Uint8 b = (Uint8)(160 + (int)((starsMass[i].mass - 80) * 0.475f));
                SDL_SetRenderDrawColor(data->SDLrenderer, b, b, (Uint8)(b * 0.7f), 255);
                SDL_FRect rect = { starsTransform[i].position.x - 1.5f, starsTransform[i].position.y - 1.5f, 3.0f, 3.0f};
                SDL_RenderFillRect(data->SDLrenderer, &rect);
                SDL_SetRenderDrawColor(data->SDLrenderer, 0, 0, 0, 0);
            }
        },
        ECS::SystemGroup::Render);
}