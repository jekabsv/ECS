#include "Level1.h"
#include "RenderComponent.h"
#include "MeshComponent.h"
#include "SimpleSprite.h"
#include "InputComponent.h"
#include "NBody.h"
#include "BenchmarkState.h"
#include "FusionBenchmark.h"

struct PlayerComponent
{
    int direction = 0;
};


void Level1::Init()
{
    //LOG_ERROR(GlobalLogger, "TEST", "test");

	ecs.Tie(_data);
	
	e = ecs.Create();
	playerEntity = ecs.Create();

    ecs.Add<RenderComponent>(e, RenderComponent(false));
    ecs.Add<MeshComponent>(e, MeshComponent("triangle"));

    ecs.Add<RenderComponent>(playerEntity, RenderComponent(true, Vec2(10, 10)));
    ecs.Add<SimpleSprite>(playerEntity, SimpleSprite({ 100, 100, 100, 100 }, { 0, 0, 64, 64 }, "player"));
    ecs.Add<InputComponent>(playerEntity, InputComponent());
    ecs.Add<PlayerComponent>(playerEntity, PlayerComponent(SDL_GetTicks()));
    ecs.Add<AnimationPlayer>(playerEntity, AnimationPlayer{});
    _data->animation.Play(ecs.Get<AnimationPlayer>(playerEntity), "player_idle_right");

    ecs.RegisterSystem<RenderComponent, SimpleSprite, InputComponent, PlayerComponent, AnimationPlayer>(
        "move",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef _data)
        {
            auto rcs = ctx.Slice<RenderComponent>();
            auto sss = ctx.Slice<SimpleSprite>();
            auto pcs = ctx.Slice<PlayerComponent>();
            auto anims = ctx.Slice<AnimationPlayer>();

            auto dMove = _data->inputs.GetActionAxis("move");
            auto dScale = _data->inputs.GetActionAxis("scale");

            for (std::size_t i = 0; i < rcs.size(); i++)
            {
                auto& rc = rcs[i];
                auto& pc = pcs[i];
                auto& anim = anims[i];

                rc.position.x += 1000 * dMove[0] * dt;
                rc.position.y += 1000 * dMove[1] * dt;
                rc.scale.x += dScale[0] * dt * 5;
                rc.scale.y += dScale[0] * dt * 5;

                if (dMove[0] < 0) pc.direction = 1;
                if (dMove[0] > 0) pc.direction = 0;

                if (!dMove[0])
                {
                    StringId idle = pc.direction ? "player_idle_left" : "player_idle_right";
                    if (anim.currentClip != idle)
                        _data->animation.Play(anim, idle);
                }
                else
                {
                    StringId run = pc.direction ? "player_run_left" : "player_run_right";
                    if (anim.currentClip != run)
                        _data->animation.Play(anim, run);
                }

                _data->animation.Update(anim, dt);

                sss[i].TextureRect = anim.currentRect;
                sss[i].TextureName = anim.currentSpritesheet;
            }
        },
        ECS::SystemGroup::Update);

    ecs.RegisterSystem<RenderComponent, SimpleSprite>("renderSprite",
        [](ECS::ArchetypeContext ctx, float dt, SharedDataRef _data)
        {
            auto rcs = ctx.Slice<RenderComponent>();
            auto sss = ctx.Slice<SimpleSprite>();

            for (std::size_t i = 0; i < rcs.size(); i++)
            {
                auto& rc = rcs[i];
                auto& ss = sss[i];

                SDL_FRect RectToDraw = ss.rect;
                RectToDraw.h *= rc.scale.y;
                RectToDraw.w *= rc.scale.x;
                RectToDraw.x += rc.position.x;
                RectToDraw.y += rc.position.y;

                if (!ss.TextureRect.h || !ss.TextureRect.w)
                    _data->renderer.Render(_data->assets.GetTexture(ss.TextureName), nullptr, &RectToDraw);
                else
                    _data->renderer.Render(_data->assets.GetTexture(ss.TextureName), &ss.TextureRect, &RectToDraw);
            }
        },
        ECS::SystemGroup::Render);
}

void Level1::Update(float dt)
{
    if(_data->inputs.GetActionState("next") == InputSystem::Pressed)
        _data->state.AddState(StateRef(new NBody(_data)), 1);
}
