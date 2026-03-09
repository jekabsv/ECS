#include "StartState.h"
#include "RenderComponent.h"
#include "MeshComponent.h"
#include "SimpleSprite.h"
#include "InputComponent.h"

class ProcessWASD : public InputSystem::Processor
{
public:
    ProcessWASD(StringId _name) : InputSystem::Processor(_name) {};
    int Process(InputSystem::INPUT_DATA_4& data) override
    {
        float x = data[3] - data[1];
        float y = data[2] - data[0];
        float lengthSq = x * x + y * y;
        if (lengthSq > 0.0001f)
        {
            float length = std::sqrt(lengthSq);
            x /= length;
            y /= length;
        }
        data[0] = x;
        data[1] = y;
        data[2] = 0.0f;
        data[3] = 0.0f;

        return 0;
    }
};
class ProcessQE : public InputSystem::Processor
{
public:
    ProcessQE(StringId _name) : InputSystem::Processor(_name) {};
    int Process(InputSystem::INPUT_DATA_4& data) override
    {
        data[0] -= data[1];
        return 0;
    }
};

struct PlayerComponent
{
    PlayerComponent(uint64_t _start) :start(_start) {};
    int direction = 0;
    int TexX = 0;
    int TexY = 0;
    uint64_t start = 0;
};


void StartState::Init()
{
    Mesh triangleMesh = {
        {-1.0f, 1.0f, 1.0f, 0.0f, 0.0f, 1.0f},
        {1.0f, 1.0f, 0.0f, 1.0f, 0.0f, 1.0f},
        {0.0f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f} 
    };

    Mesh squareMesh = {
    {{-1,  1}, {1.0f, 0.0f, 0.0f, 1.0f}},
    {{-1, -1}, {0.0f, 1.0f, 0.0f, 1.0f}},
    {{ 1,  1}, {0.0f, 0.0f, 1.0f, 1.0f}},

    {{ 1, -1}, {1.0f, 1.0f, 0.0f, 1.0f}},
    {{-1, -1}, {0.0f, 1.0f, 0.0f, 1.0f}},
    {{ 1,  1}, {0.0f, 0.0f, 1.0f, 1.0f}}
    };

    ecs.Tie(_data);

    _data->assets.AddMesh("Triangle", triangleMesh);
    _data->assets.AddMesh("square", squareMesh);

    _data->assets.LoadBMPTexture("test", "../image.bmp", _data->SDLrenderer);
    _data->assets.LoadBMPTexture("player", "../player.bmp", _data->SDLrenderer);

    e = ecs.Create();
    playerEntity = ecs.Create();

    ecs.Add<RenderComponent>(e, RenderComponent(false));
    ecs.Add<MeshComponent>(e, MeshComponent("triangle", ""));
    
    ecs.Add<RenderComponent>(playerEntity, RenderComponent(true, Vec2(10, 10)));
    ecs.Add<SimpleSprite>(playerEntity, SimpleSprite({100, 100, 100, 100}, {0, 0, 64, 64}, "player"));
    ecs.Add<InputComponent>(playerEntity, InputComponent());
    ecs.Add<PlayerComponent>(playerEntity, PlayerComponent(SDL_GetTicks()));

    _data->inputs.AddActionMap("gameplay").AddAction("move")
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_W, 0)
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_A, 1)
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_S, 2)
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_D, 3)
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_UP, 0)
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_LEFT, 1)
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_DOWN, 2)
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_RIGHT, 3)
        .AddProcessor(std::make_unique<ProcessWASD>("wasd"));

    _data->inputs.AssignDeviceToPlayer(InputSystem::KeyboardHub::Current());
    _data->inputs.AssignMapToPlayer("gameplay");

    _data->inputs.GetActionMap("gameplay")->AddAction("scale")
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_Q, 0)
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_E, 1)
        .AddProcessor(std::make_unique<ProcessQE>("qe"));

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