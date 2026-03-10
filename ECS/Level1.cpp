#include "Level1.h"
#include "RenderComponent.h"
#include "MeshComponent.h"
#include "SimpleSprite.h"
#include "InputComponent.h"
#include "NBody.h"

struct PlayerComponent
{
    PlayerComponent(uint64_t _start) :start(_start) {};
    int direction = 0;
    int TexX = 0;
    int TexY = 0;
    uint64_t start = 0;
};


void Level1::Init()
{
	ecs.Tie(_data);
	
	e = ecs.Create();
	playerEntity = ecs.Create();

    ecs.Add<RenderComponent>(e, RenderComponent(false));
    ecs.Add<MeshComponent>(e, MeshComponent("triangle", ""));

    ecs.Add<RenderComponent>(playerEntity, RenderComponent(true, Vec2(10, 10)));
    ecs.Add<SimpleSprite>(playerEntity, SimpleSprite({ 100, 100, 100, 100 }, { 0, 0, 64, 64 }, "player"));
    ecs.Add<InputComponent>(playerEntity, InputComponent());
    ecs.Add<PlayerComponent>(playerEntity, PlayerComponent(SDL_GetTicks()));

    ecs.RegisterSystem<RenderComponent, SimpleSprite, InputComponent, PlayerComponent>("move",
        [](ECS::Entity entity, ECS::ComponentContext context, float dt, SharedDataRef _data)
        {
            auto& rc = context.Get<RenderComponent>();
            auto& ss = context.Get<SimpleSprite>();
            auto& pc = context.Get<PlayerComponent>();

            InputSystem::INPUT_DATA_4 dMove = _data->inputs.GetActionAxis("move");
            InputSystem::INPUT_DATA_4 dScale = _data->inputs.GetActionAxis("scale");

            rc.position.x += 1000 * dMove[0] * dt;
            rc.position.y += 1000 * dMove[1] * dt;
            rc.scale.x += dScale[0] * dt * 5;
            rc.scale.y += dScale[0] * dt * 5;

            if (dMove[0] < 0)
                pc.direction = 1;
            if (dMove[0] > 0)
                pc.direction = 0;


            if (!dMove[0])
            {
                if (!pc.direction)
                {
                    pc.TexX = 3;
                    pc.TexY = 1;
                }
                if (pc.direction)
                {
                    pc.TexX = 3;
                    pc.TexY = 3;
                }
            }
            else
            {
                if (!pc.direction)
                {
                    pc.TexY = 0;
                }
                if (pc.direction)
                {
                    pc.TexY = 2;
                }
                if (SDL_GetTicks() - pc.start > 100)
                {
                    pc.TexX++;
                    pc.start = SDL_GetTicks();
                    pc.TexX %= 8;
                }
            }

            ss.TextureRect.x = pc.TexX * 64;
            ss.TextureRect.y = pc.TexY * 64;
        },
        ECS::SystemGroup::Update);

    ecs.RegisterSystem<RenderComponent, SimpleSprite>("renderSprite",
        [](ECS::Entity entity, ECS::ComponentContext context, float dt, SharedDataRef _data)
        {
            auto& rc = context.Get<RenderComponent>();
            auto& ss = context.Get<SimpleSprite>();

            SDL_FRect RectToDraw = ss.rect;
            RectToDraw.h *= rc.scale.y;
            RectToDraw.w *= rc.scale.x;
            RectToDraw.x += rc.position.x;
            RectToDraw.y += rc.position.y;

            if (!ss.TextureRect.h || !ss.TextureRect.w)
                _data->renderer.Render(_data->assets.GetTexture(ss.TextureName), nullptr, &RectToDraw);
            else
                _data->renderer.Render(_data->assets.GetTexture(ss.TextureName), &ss.TextureRect, &RectToDraw);
        },
        ECS::SystemGroup::Render);


}

void Level1::Update(float dt)
{
    if(_data->inputs.GetActionState("next") == InputSystem::Pressed)
        _data->state.AddState(StateRef(new NBody(_data)), 1);
}
