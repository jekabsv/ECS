#include "Level1.h"
#include "MeshComponent.h"
#include "SimpleSprite.h"
#include "InputComponent.h"
#include "Transform.h"
#include "Utility.h"

#include <Windows.h>
#include <iostream>

struct Clicked { bool clicked = false; };

void Level1::Init()
{
    _data->physics.EnableCollisionDetection(true);
    _data->physics.EnableMovement();

    e = ecs.Create();
    playerEntity = ecs.Create();

    ecs.Add<BoxCollider>(e, BoxCollider(100.0f, 100.0f, true));
    ecs.Add<MeshComponent>(e, MeshComponent("tri", "mat", true));
    ecs.Add<TransformComponent>(e, TransformComponent({ 500.0f, 500.0f , 0.f}, { 1.0f, 1.0f }));
    ecs.Add<Clicked>(e, Clicked());



    ecs.Add<SimpleSprite>(playerEntity, SimpleSprite({ 0, 0, 64, 64 }, "sprite_mat", true, "player"));
    ecs.Add<TransformComponent>(playerEntity, TransformComponent({ 0.0f, 0.0f , 0.f}, { 1.0f, 1.0f }));
    ecs.Add<InputComponent>(playerEntity, InputComponent());
    ecs.Add<AnimationPlayer>(playerEntity, AnimationPlayer{});
    ecs.Add<BoxCollider>(playerEntity, BoxCollider(24.0f, 32.0f, true));
    ecs.Add<Clicked>(playerEntity, Clicked());

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

                sprite.TextureSRect = anim.currentRect;
                sprite.material.textures[0] = anim.currentSpritesheet;
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




    ecs.RegisterSystem<Clicked, TransformComponent>("click",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef data)
        {
            auto transforms = ctx.Slice<TransformComponent>();
            auto click = ctx.Slice<Clicked>();
            for (std::size_t i = 0; i < transforms.size(); i++)
            {
                if (click[i].clicked)
                {
                    transforms[i].position.x = data->inputs.GetActionAxis("mousePos")[0];
                    transforms[i].position.y = data->inputs.GetActionAxis("mousePos")[1];
                    click[i].clicked = false;
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

                sprite.material.ClearUniforms();

                _data->renderer.SubmitSprite(sprite.material, sprite.TextureSRect, transform.position, transform.scale, transform.rotation);
            }
        },
        ECS::SystemGroup::Render);

    ecs.RegisterSystem<MeshComponent, TransformComponent>("renderMesh",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef _data)
        {
            auto meshes = ctx.Slice<MeshComponent>();
            auto transforms = ctx.Slice<TransformComponent>();

            for (int i = 0; i < meshes.size(); i++)
            {
                auto& meshC = meshes[i];
                auto& transform = transforms[i];

                if (!meshC.render)
                    continue;

                const MeshBase* mesh = _data->assets.GetMesh(meshC.MeshName.meshName);
                if (!mesh)
                    continue;

                _data->renderer.SubmitMesh(meshC.MeshName, meshC.Material, transform.position, transform.scale, transform.rotation);
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

                _data->renderer.SubmitMesh(MeshInstance("unit_quad"), MaterialInstance("mat_transp"),
                    { ax + aw / 2.f, ay + ah / 2.f , 0.4f}, { aw, ah }, 0.0f, { 1.f, 0.f, 0.f, 0.3f });
            }
        },
        ECS::SystemGroup::Render);



    ui.GetTheme().LoadDarkDefaults();

    //ui.RegisterFont("main", _data->assets.GetFont("main"));


    UI::NodeHandle root = ui.AddContainer();
    ui.SetSize(root, UI::SizeValue::Px(500), UI::SizeValue::Auto());
    ui.SetFlexDirection(root, UI::FlexDirection::Column);
    ui.SetJustify(root, UI::JustifyContent::FlexStart);
    ui.SetAlignItems(root, UI::AlignItems::Stretch);
    ui.SetGap(root, 12.0f);
    ui.SetPadding(root, UI::Edges::All(20.0f));

    back = ui.AddButton("Back to menu", root);
}

void Level1::Update(float dt)
{
    if (ui.IsClicked(back))
        _data->state.RemoveState();

    if (_data->inputs.GetActionState("click") == InputSystem::Held)
    {
        float mx = _data->inputs.GetActionAxis("mousePos")[0];
        float my = _data->inputs.GetActionAxis("mousePos")[1];

        std::vector<ECS::Entity> foundEntities;
        _data->spatialIndex.QueryPoint(mx, my, foundEntities);

        for (auto e : foundEntities)
        {
            auto col = ecs.TryGet<BoxCollider>(e);
            auto pos = ecs.TryGet<TransformComponent>(e);
            auto click = ecs.TryGet<Clicked>(e);
            if (col && pos && click)
                if (Utility::Contains(mx, my, pos, col)) {
                    click->clicked = true;
                    std::cout << "  clicked on entity: " << e << '\n';
                }
        }
    }


    if (_data->inputs.GetActionState("next") == InputSystem::Pressed)
        _data->state.RemoveState();

    if (_data->inputs.GetActionState("show_colliders") == InputSystem::Pressed)
        ecs.ToggleSystem("draw_colliders");
}