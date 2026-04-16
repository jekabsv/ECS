#include "StartState.h"
#include "Level1.h"
#include <windows.h>
#include "SPH.h"
#include "Boids.h"
#include "Uicontext.h";
#include <iostream>

class ProcessWASD : public InputSystem::Processor
{
public:
    using InputSystem::Processor::Processor;
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




void StartState::Init()
{

    //Shaders
    _data->assets.LoadShader(StringId("vertSprite"), "../ECS/vertSprite.spv", _data->device, ShaderStage::VERTEX,
        /*samplers*/0, /*storageTex*/0, /*storageBuf*/0, /*uniformBufs*/3);

    _data->assets.LoadShader(StringId("fragSprite"), "../ECS/fragSprite.spv", _data->device, ShaderStage::FRAGMENT,
        /*samplers*/1, /*storageTex*/0, /*storageBuf*/0, /*uniformBufs*/0);


    _data->assets.LoadShader(StringId("vert"), "../ECS/vert.spv", _data->device, ShaderStage::VERTEX,
        /*samplers*/0, /*storageTex*/0, /*storageBuf*/0, /*uniformBufs*/2);

    _data->assets.LoadShader(StringId("frag"), "../ECS/frag.spv", _data->device, ShaderStage::FRAGMENT,
        /*samplers*/0, /*storageTex*/0, /*storageBuf*/0, /*uniformBufs*/0);



	//Textures
    _data->assets.LoadBMPSurface("test", "../ECS/test.bmp");
	_data->assets.AddTexture("test", _data->renderer.CreateTexture(_data->assets.GetSurface("test")));

    _data->assets.LoadBMPSurface("player", "../ECS/player.bmp");
    _data->assets.AddTexture("player", _data->renderer.CreateTexture(_data->assets.GetSurface("player")));



    MaterialBase spriteMaterial("vertSprite", "fragSprite");
    MaterialBase::MakeSpriteTransparent(spriteMaterial);
    MaterialBase::SetSDL_VertexAttr(spriteMaterial);


    MaterialBase mat("vert", "frag");
    MaterialBase::MakeSpriteTransparent(mat);
    MaterialBase::SetSDL_VertexAttr(mat);

    _data->assets.AddMaterial(StringId("tri_mat"), mat);
    




    MeshVertices verts = {
    { { 0.0f, -100.0f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.5f, 0.0f} }, // Top
    { { 100.0f, 100.0f}, {0.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f} }, // Right
    { {-100.0f, 100.0f}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 1.0f} }  // Left
    };
    MeshIndices indices = { 0, 1, 2 };

    _data->assets.AddMesh(StringId("tri"), verts, indices);




    //Inputs
    _data->assets.AddMaterial(StringId("sprite_mat"), spriteMaterial);


    _data->inputs.AddActionMap("level1").AddAction("move")
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_W, 0)
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_A, 1)
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_S, 2)
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_D, 3)
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_UP, 0)
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_LEFT, 1)
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_DOWN, 2)
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_RIGHT, 3)
        .AddProcessor(std::make_unique<ProcessWASD>("wasd"));


    _data->inputs.GetActionMap("level1")->AddAction("click")
        .AddBinding(InputSystem::Button, InputSystem::Mouse, SDL_BUTTON_LEFT);

    _data->inputs.AssignMapToPlayer("level1");

    _data->inputs.AssignDeviceToPlayer(InputSystem::KeyboardHub::Current());
}

void StartState::Update(float dt)
{
    auto axis = _data->inputs.GetActionAxis("move");
    x += axis[0] * 50.f * dt;
    y -= axis[1] * 50.f * dt;

    rotationAngle += dt * 2.0f;
}

void StartState::Render(float dt)
{
    MaterialInstance spriteMatInst{ StringId("sprite_mat") };
    spriteMatInst.textures.push_back(StringId("player"));

    float TexX = (int)(x/10.f) * 64.f;
        
    _data->renderer.SpriteDraw(spriteMatInst, { TexX, 0.f, 64.f, 64.f }, { x, y });


    MeshInstance meshInst{ StringId("tri") };
    MaterialInstance matInst{ StringId("tri_mat") };

    _data->renderer.DrawMesh(meshInst, matInst, { 400.f, 300.f }, { 1.0f, 1.0f }, rotationAngle, { 1.f, 1.f, 1.f, 0.8f });
}

