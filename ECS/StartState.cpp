#include "StartState.h"
#include "RenderComponent.h"
#include "MeshComponent.h"
#include "SimpleSprite.h"

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

    _data->assets.AddMesh(0, triangleMesh);

    Mesh squareMesh = {
    {{-1,  1}, {1.0f, 0.0f, 0.0f, 1.0f}},
    {{-1, -1}, {0.0f, 1.0f, 0.0f, 1.0f}},
    {{ 1,  1}, {0.0f, 0.0f, 1.0f, 1.0f}},

    {{ 1, -1}, {1.0f, 1.0f, 0.0f, 1.0f}},
    {{-1, -1}, {0.0f, 1.0f, 0.0f, 1.0f}},
    {{ 1,  1}, {0.0f, 0.0f, 1.0f, 1.0f}} 
    };

    _data->assets.AddMesh(1, squareMesh);




    e = ecs.create_entity();
    ecs.add_component(e, RenderComponent(false));
    ecs.add_component(e, MeshComponent(TRIANGLE, -1));
    
    player = ecs.create_entity();
    ecs.add_component(player, RenderComponent(true));
    ecs.add_component(player, SimpleSprite({100, 100, 100, 100}, {0, 0, 64, 64}, 2));

    _data->assets.LoadBMPTexture(1, "C:/Users/jekabs.vidrusks/Downloads/images.bmp", _data->renderer);
    _data->assets.LoadBMPTexture(2, "C:/Users/jekabs.vidrusks/Downloads/player.bmp", _data->renderer);

    RenderComponent& rc = ecs.get<RenderComponent>(e);
    rc.scale = { 10, 10 };
    rc.Position = { 10, 10 };

}

float dx = 0;
float dy = 0;
bool direction = 0;
int TexX;
int TexY;

Uint32 start = SDL_GetTicks();




void StartState::Update()
{

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_EVENT_QUIT)
            _data->quit = true;
    }

    const bool* key_states = SDL_GetKeyboardState(NULL);
    RenderComponent& rc = ecs.get<RenderComponent>(player);

    dx = 0;
    dy = 0;

    if (key_states[SDL_SCANCODE_W]) {
        dy -= 1;
    }
    if (key_states[SDL_SCANCODE_S]) {
        dy += 1;
    }
    if (key_states[SDL_SCANCODE_A]) {
        dx -= 1;
    }
    if (key_states[SDL_SCANCODE_D]) {
        dx += 1;
    }
    if (key_states[SDL_SCANCODE_E]) {
        rc.scale.x -= 1.0f;
        rc.scale.y -= 1.0f;
    }
    if (key_states[SDL_SCANCODE_Q]) {
        rc.scale.x += 1.0f;
        rc.scale.y += 1.0f;
    }


    rc.Position.x += 10 * dx;
    rc.Position.y += 10 * dy;

    

    if (dx < 0)
        direction = 1;
    if (dx > 0)
        direction = 0;
    
    
    if (!dx)
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
    /*rc.Position.x;
    rc.Position.y;*/


    /*if (rc.Position.x + rc.scale.x >= 1920 || rc.Position.x - rc.scale.x <= 0)
        speedx *= -1;
    if (rc.Position.y + rc.scale.y >= 1080 || rc.Position.y - rc.scale.y <= 0)
        speedy *= -1;*/


    //if (key_states[SDL_SCANCODE_UP]) {
    //    if(speedx < 0)
    //        speedx -= 0.1f;
    //    else speedx += 0.1f;
    //    if (speedy < 0)
    //        speedy -= 0.1f;
    //    else speedy += 0.1f;
    //}
    //if (key_states[SDL_SCANCODE_DOWN]) {
    //    if (speedx > 0)
    //        speedx -= 0.1f;
    //    else speedx += 0.1f;
    //    if (speedy > 0)
    //        speedy -= 0.1f;
    //    else speedy += 0.1f;
    //    /*speedx -= 0.1f;
    //    speedy -= 0.1f;*/
    //}


    
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
                        RectToDraw.x += rc->Position.x;
                        RectToDraw.y += rc->Position.y;

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
                                mesh_toDraw[i].position.x += rc->Position.x;

                                mesh_toDraw[i].position.y *= rc->scale.y;
                                mesh_toDraw[i].position.y += rc->Position.y;

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

