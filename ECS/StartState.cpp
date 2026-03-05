#include "StartState.h"
#include "RenderComponent.h"
#include "MeshComponent.h"
#include "SimpleSprite.h"

class ProcessWASD : public InputSystem::Processor
{
public:
    ProcessWASD(std::string _name) : InputSystem::Processor(_name) {};
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
    ProcessQE(std::string _name) : InputSystem::Processor(_name) {};
    int Process(InputSystem::INPUT_DATA_4& data) override
    {
        data[0] -= data[1];
        return 0;
    }
};

enum Mesh_names {
    TRIANGLE = 0,
    SQUARE = 1
};

ecs::Entity e;
ecs::Entity player;

float speedy;
float speedx;

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


    _data->assets.AddMesh(TRIANGLE, triangleMesh);
    _data->assets.AddMesh(SQUARE, squareMesh);

    _data->assets.LoadBMPTexture(1, "../image.bmp", _data->renderer);
    _data->assets.LoadBMPTexture(2, "../player.bmp", _data->renderer);

    e = ecs.create_entity();
    player = ecs.create_entity();

    ecs.add_component(e, RenderComponent(false));
    ecs.add_component(e, MeshComponent(TRIANGLE, -1));
    
    ecs.add_component(player, RenderComponent(true, Vec2(10, 10), Vec2(1, 1)));
    ecs.add_component(player, SimpleSprite({100, 100, 100, 100}, {0, 0, 64, 64}, 2));


    _data->inputs.AddActionMap("gameplay").AddAction("move")
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_W, 0)
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_A, 1)
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_S, 2)
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_D, 3)
        .AddProcessor(std::make_unique<ProcessWASD>("wasd"));

    _data->inputs.AssignDeviceToPlayer(player, InputSystem::KeyboardHub::Current());
    _data->inputs.AssignMapToPlayer(player, "gameplay");

    _data->inputs.GetActionMap("gameplay").AddAction("scale")
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_Q, 0)
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_E, 1)
        .AddProcessor(std::make_unique<ProcessQE>("qe"));
}




int TexX;
int TexY;
bool direction = 0;

Uint32 start = SDL_GetTicks();

void StartState::Update()
{
    RenderComponent& rc = ecs.get<RenderComponent>(player);

    InputSystem::INPUT_DATA_4 dMove = _data->inputs.GetActionAxis("move", player);
    InputSystem::INPUT_DATA_4 dScale = _data->inputs.GetActionAxis("scale", player);

    rc.position.x += 10 * dMove[0];
    rc.position.y += 10 * dMove[1];

    rc.scale.x += dScale[0];
    rc.scale.y += dScale[0];


    if (dMove[0] < 0)
        direction = 1;
    if (dMove[0] > 0)
        direction = 0;
    
    
    if (!dMove[0])
    {
        if (!direction)
        {
            TexX = 3;
            TexY = 1;
        }
        if (direction)
        {
            TexX = 3;
            TexY = 3;
        }
    }
    else
    {
        if (!direction)
        {
            TexY = 0;
        }
        if (direction)
        {
            TexY = 2;
        }
        if (SDL_GetTicks() - start > 100)
        {
            TexX++;
            start = SDL_GetTicks();
            TexX %= 8;
        }
    }
        


    SimpleSprite& ss = ecs.get<SimpleSprite>(player);
    ss.TextureRect.x = TexX * 64;
    ss.TextureRect.y = TexY * 64;
}




void StartState::Render()
{

    if (!_data->renderer)
    {
        std::cout << SDL_GetError() << '\n';
        return;
    }

    SDL_RenderClear(_data->renderer);


        ecs.for_each(ecs.CreateMask<RenderComponent>(), [this](ecs::Entity e, std::function<void* (ecs::TypeId)> getComponent)
            {
                RenderComponent* rc = reinterpret_cast<RenderComponent*>(getComponent(ecs::getTypeId<RenderComponent>()));
                if (rc->render)
                {
                    SimpleSprite* ss = reinterpret_cast<SimpleSprite*>(getComponent(ecs::getTypeId<SimpleSprite>()));
                    if (ss)
                    {
                        SDL_FRect RectToDraw = ss->rect;
                        RectToDraw.h *= rc->scale.y;
                        RectToDraw.w *= rc->scale.x;
                        RectToDraw.x += rc->position.x;
                        RectToDraw.y += rc->position.y;

                        if(!ss->TextureRect.h || !ss->TextureRect.w)
                            SDL_RenderTexture(_data->renderer, _data->assets.GetTexture(ss->TextureId), nullptr, &RectToDraw);
                        else
                            SDL_RenderTexture(_data->renderer, _data->assets.GetTexture(ss->TextureId), &ss->TextureRect, &RectToDraw);
                    }
                    else
                    {
                        MeshComponent* mc = reinterpret_cast<MeshComponent*>(getComponent(ecs::getTypeId<MeshComponent>()));
                        if (mc)
                        {
                            Mesh mesh_toDraw = _data->assets.GetMesh(mc->meshId);
                            for (int i = 0; i < mesh_toDraw.size(); i++)
                            {
                                mesh_toDraw[i].position.x *= rc->scale.x;
                                mesh_toDraw[i].position.x += rc->position.x;

                                mesh_toDraw[i].position.y *= rc->scale.y;
                                mesh_toDraw[i].position.y += rc->position.y;

                                mesh_toDraw[i].color;
                                mesh_toDraw[i].tex_coord;
                            }
                            SDL_RenderGeometry(_data->renderer, _data->assets.GetTexture(mc->TextureId), mesh_toDraw.data(), mesh_toDraw.size(), nullptr, 0);
                        }
                    }
                    

                    
                    

                }
            });

    SDL_RenderPresent(_data->renderer);
    return;
}

