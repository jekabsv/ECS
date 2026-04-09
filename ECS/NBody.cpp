#include "NBody.h"
#include <cstdlib>
#include <cmath>
#include "Transform.h"
#include "Level1.h"


void NBody::Init()
{
    _data->physics.EnableCollisionDetection();

    const float W = (float)_data->GAME_WIDTH;
    const float H = (float)_data->GAME_HEIGHT * 0.5f;

    for (int i = 0; i < N; i++)
    {
        ECS::Entity e = ecs.Create();

        RigidBody rb;
        StarComponent star;
        TransformComponent tr;

        rb.drag = 0.1f;
        rb.isStatic = false;
        rb.vx = ((rand() % 400) - 200) * 0.3f;
        rb.vy = ((rand() % 400) - 200) * 0.3f;

        tr.position.x = (float)(rand() % (int)W);
        tr.position.y = (float)(rand() % (int)H);
        
        star.mass = 80.0f + (float)(rand() % 120);
        
        ecs.Add<StarComponent>(e, star);
        ecs.Add<TransformComponent>(e, tr);
        ecs.Add<RigidBody>(e, rb);
        ecs.Add<BoxCollider>(e, BoxCollider(1.5f, 1.5f));
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


    ecs.RegisterSystem<BoxCollider, TransformComponent>("draw_colliders",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef _data)
        {
            auto colliders = ctx.Slice<BoxCollider>();
            auto transforms = ctx.Slice<TransformComponent>();

            for (int i = 0; i < colliders.size(); i++)
            {
                auto& transform = transforms[i];
                auto& col = colliders[i];
                float ax = transform.position.x + col.offsetX * transform.scale.x - col.hw * transform.scale.x;
                float ay = transform.position.y + col.offsetY * transform.scale.y - col.hh * transform.scale.y;
                float aw = col.hw * 2.0f * transform.scale.x;
                float ah = col.hh * 2.0f * transform.scale.y;


                SDL_FRect rect = { ax, ay, aw, ah };
                SDL_SetRenderDrawColor(_data->SDLrenderer, 255, 0, 0, 255);
                SDL_RenderRect(_data->SDLrenderer, &rect);
                SDL_SetRenderDrawColor(_data->SDLrenderer, 0, 0, 0, 255);

            }
        },
        ECS::SystemGroup::Render);
}



void NBody::Update(float dt)
{
    if(_data->inputs.GetActionState("show_colliders") == InputSystem::Pressed)
    {
        running = !running;
        _data->physics.EnableMovement(running);
        //ecs.ToggleSystem("draw_colliders");/
    }
    if (_data->inputs.GetActionState("next") == InputSystem::Pressed)
        _data->state.AddState(StateRef(new Level1(_data)), 1);
}


/*ecs.RegisterSystem<BoxCollider, TransformComponent, RigidBody>("onCollision",
    [](ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
    {
        auto transforms = ctx.Slice<TransformComponent>();
        auto entities = ctx.Slice<ECS::Entity>();
        auto rbs = ctx.Slice<RigidBody>();
        auto colliders = ctx.Slice<BoxCollider>();

        for (std::size_t i = 0; i < transforms.size(); i++)
        {
            auto& ev = data->physics.GetContacts(entities[i]);
            for (auto other : ev)
            {
                // 1. Get Other's Data
                auto* otherTr = data->physics.GetWorld()->TryGet<TransformComponent>(other);
                auto* otherCol = data->physics.GetWorld()->TryGet<BoxCollider>(other);
                auto* otherRb = data->physics.GetWorld()->TryGet<RigidBody>(other);

                if (!otherTr || !otherCol || otherCol->isTrigger) continue;

                // 2. Calculate Overlap (Manifold)
                float dx = otherTr->position.x - transforms[i].position.x;
                float px = (otherCol->hw + colliders[i].hw) - std::abs(dx);

                float dy = otherTr->position.y - transforms[i].position.y;
                float py = (otherCol->hh + colliders[i].hh) - std::abs(dy);

                if (px <= 0 || py <= 0) continue;

                // 3. Positional Correction (The "De-Penetration")
                // We move them out along the shallowest axis
                if (px < py) {
                    float sign = (dx > 0) ? -1.0f : 1.0f;
                    transforms[i].position.x += px * sign;
                    // 4. Velocity Reflection (Impulse)
                    rbs[i].vx *= -0.5f; // Add "bounciness" factor (0.5 = lose half energy)
                }
                else {
                    float sign = (dy > 0) ? -1.0f : 1.0f;
                    transforms[i].position.y += py * sign;
                    rbs[i].vy *= -0.5f;
                }
            }
        }
    },
    ECS::SystemGroup::Update);*/
