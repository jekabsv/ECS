#include "Level1.h"
#include "MeshComponent.h"
#include "SimpleSprite.h"
#include "InputComponent.h"
#include "NBody.h"
#include "Transform.h"
#include "physSim.h"
#include <iostream>

void Level1::Init()
{
    _data->physics.EnableCollisionDetection();
    _data->physics.EnableMovement();
    _data->inputs.AssignMapToPlayer("level1");

	e = ecs.Create();
	playerEntity = ecs.Create();

    ecs.Add<BoxCollider>(e, BoxCollider(1.0f, 1.0f, true));
    ecs.Add<MeshComponent>(e, MeshComponent("Triangle", "", true));
    ecs.Add<TransformComponent>(e, TransformComponent({ 500.0f, 500.0f }, { 100.0f, 100.0f }));


    ecs.Add<SimpleSprite>(playerEntity, SimpleSprite({ -50, -50, 100, 100 }, { 0, 0, 64, 64 }, "player", true));
    ecs.Add<TransformComponent>(playerEntity, TransformComponent({ 0.0f, 0.0f }, { 1.0f, 1.0f }));
    ecs.Add<InputComponent>(playerEntity, InputComponent());
    ecs.Add<AnimationPlayer>(playerEntity, AnimationPlayer{});
    ecs.Add<BoxCollider>(playerEntity, BoxCollider(25.0f, 50.0f, true));


    _data->animation.Play(ecs.Get<AnimationPlayer>(playerEntity), "player_idle_right");

    ecs.RegisterSystem<TransformComponent, SimpleSprite, InputComponent, AnimationPlayer>(
        "move",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef _data)
        {
            auto anims = ctx.Slice<AnimationPlayer>();
            auto transforms = ctx.Slice<TransformComponent>();
            auto sprites = ctx.Slice<SimpleSprite>();

            auto dMove = _data->inputs.GetActionAxis("move");
            auto dScale = _data->inputs.GetActionAxis("scale");

            for (std::size_t i = 0; i < sprites.size(); i++)
            {
                auto& sprite = sprites[i];
                auto& anim = anims[i];
                auto& transform = transforms[i];

                transform.position.x += 1000 * dMove[0] * dt;
                transform.position.y += 1000 * dMove[1] * dt;
                transform.scale.x += dScale[0] * dt * 5;
                transform.scale.y += dScale[0] * dt * 5;
                

                if (!dMove[0])
                {
                    if (anim.currentClip == "player_run_left")
                        _data->animation.Play(anim, "player_idle_left");
                    else if (anim.currentClip == "player_run_right")
                        _data->animation.Play(anim, "player_idle_right");
                }
                else
                {
                    if (dMove[0] < 0 && anim.currentClip != "player_run_left")
                        _data->animation.Play(anim, "player_run_left");
                    if (dMove[0] > 0 && anim.currentClip != "player_run_right")
                        _data->animation.Play(anim, "player_run_right");
                }

                _data->animation.Update(anim, dt);

                sprite.TextureRect = anim.currentRect;
                sprite.TextureName = anim.currentSpritesheet;
            }
        },
        ECS::SystemGroup::Update);

    ecs.RegisterSystem<BoxCollider, TransformComponent>("onCollision",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
        {
            auto transforms = ctx.Slice<TransformComponent>();
            auto entities = ctx.Slice<ECS::Entity>();
            for (std::size_t i = 0; i < transforms.size(); i++)
            {
                auto& ev = data->physics.GetContacts(entities[i]);
                if (ev.size() == 0)
                    continue;
                for (std::size_t j = 0; j < ev.size(); j++)
                {
                    ECS::Entity other = ev[j];

                    BoxCollider* otherBc = data->physics.GetWorld()->TryGet<BoxCollider>(other);
                    if (otherBc && otherBc->isTrigger)
                    {
                        LOG_DEBUG(GlobalLogger(), "Collision", "Trigger collision!");
                    }
                    else
                    {
                        std::cout << "physics collision!\n";
                    }
                }
            }
        },
        ECS::SystemGroup::Update);




    ecs.RegisterSystem<SimpleSprite, TransformComponent>("renderSprite",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef _data)
        {
            auto sprites = ctx.Slice<SimpleSprite>();
            auto transforms = ctx.Slice<TransformComponent>();

            for (std::size_t i = 0; i < transforms.size(); i++)
            {
                auto& transform = transforms[i];
                auto& sprite = sprites[i];

                if (!sprite.render)
                    continue;
                SDL_FRect RectToDraw = sprite.rect;
                RectToDraw.h *= transform.scale.y;
                RectToDraw.w *= transform.scale.x;
                RectToDraw.x *= transform.scale.x;
                RectToDraw.y *= transform.scale.y;
                RectToDraw.x += transform.position.x;
                RectToDraw.y += transform.position.y;

                if (!sprite.TextureRect.h || !sprite.TextureRect.w)
                    _data->renderer.Render(_data->assets.GetTexture(sprite.TextureName), nullptr, &RectToDraw);
                else
                    _data->renderer.Render(_data->assets.GetTexture(sprite.TextureName), &sprite.TextureRect, &RectToDraw);
            }
        },
        ECS::SystemGroup::Render);

    ecs.RegisterSystem<MeshComponent, TransformComponent>("renderMesh",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef _data)
        {
            auto meshes = ctx.Slice<MeshComponent>();
            auto transforms = ctx.Slice<TransformComponent>();

            for (int i = 0;i < meshes.size();i++)
            {
                auto& meshC = meshes[i];
                auto& transform = transforms[i];

                if (!meshC.render)
                    continue;

                const Mesh* mesh = _data->assets.GetMesh(meshC.MeshName);
                if (!mesh)
                    continue;

                const SDL_Texture* texture = _data->assets.GetTexture(meshC.TextureName);

                std::vector<SDL_Vertex> transformed = *mesh;
                for (auto& v : transformed)
                {
                    v.position.x = v.position.x * transform.scale.x + transform.position.x;
                    v.position.y = v.position.y * transform.scale.y + transform.position.y;
                }

                SDL_RenderGeometry(
                    _data->SDLrenderer,
                    const_cast<SDL_Texture*>(texture),
                    transformed.data(),
                    (int)transformed.size(),
                    nullptr,
                    0
                );
            }
        },
        ECS::SystemGroup::Render);

    ecs.RegisterSystem<BoxCollider, TransformComponent>("draw_colliders",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef _data)
        {
            auto colliders = ctx.Slice<BoxCollider>();
            auto transforms = ctx.Slice<TransformComponent>();

            for (int i = 0;i < colliders.size();i++)
            {
                auto& transform = transforms[i];
                auto& col = colliders[i];
                float ax = transform.position.x + col.offsetX * transform.scale.x - col.hw * transform.scale.x;
                float ay = transform.position.y + col.offsetY * transform.scale.y - col.hh * transform.scale.y;
                float aw = col.hw * 2.0f * transform.scale.x;
                float ah = col.hh * 2.0f * transform.scale.y;


                SDL_FRect rect = {ax, ay, aw, ah};
                SDL_SetRenderDrawColor(_data->SDLrenderer, 255, 0, 0, 255);
                SDL_RenderRect(_data->SDLrenderer, &rect);
                SDL_SetRenderDrawColor(_data->SDLrenderer, 0, 0, 0, 255);

            }
        }, 
        ECS::SystemGroup::Render);
}

void Level1::Update(float dt)
{
    if(_data->inputs.GetActionState("next") == InputSystem::Pressed)
        _data->state.AddState(StateRef(new PhysSim(_data)), 1);

    if (_data->inputs.GetActionState("show_colliders") == InputSystem::Pressed)
        ecs.ToggleSystem("draw_colliders");
}
