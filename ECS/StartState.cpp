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


    auto  _swapchainFormat = SDL_GetGPUSwapchainTextureFormat(_data->device, _data->window);

    _data->assets.LoadShader(StringId("vert"), "../ECS/vert.spv", _data->device, ShaderStage::VERTEX,
        /*samplers*/0, /*storageTex*/0, /*storageBuf*/0, /*uniformBufs*/2);

    _data->assets.LoadShader(StringId("frag"), "../ECS/frag.spv", _data->device, ShaderStage::FRAGMENT,
        /*samplers*/0, /*storageTex*/0, /*storageBuf*/0, /*uniformBufs*/0);




    MaterialBase mat = {};
    mat.vertexShader = StringId("vert");
    mat.fragmentShader = StringId("frag");
    mat.blendMode = BlendMode::None;
    mat.colorTargetFormat = _swapchainFormat;
    mat.hasDepthTarget = true;
    mat.depthStencilFormat = SDL_GPU_TEXTUREFORMAT_D16_UNORM;
    mat.depthTestEnabled = false;
    mat.depthWriteEnabled = false;
    mat.primitiveType = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;
    mat.cullMode = SDL_GPU_CULLMODE_NONE;
    mat.fillMode = SDL_GPU_FILLMODE_FILL;
    mat.pipeline = nullptr;


    mat.attributes = {
        { 0, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,  offsetof(SDL_Vertex, position)  },
        { 1, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,  offsetof(SDL_Vertex, color)     },
        { 2, 0, SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2,  offsetof(SDL_Vertex, tex_coord) },
    };


    _data->assets.AddMaterial(StringId("tri_mat"), mat);

    MeshVertices verts = {
     { { 0.0f,  0.5f}, {1.0f, 0.0f, 0.0f, 1.0f}, {0.5f, 0.0f} }, // Top
     { { 0.5f, -0.5f}, {0.0f, 1.0f, 0.0f, 1.0f}, {1.0f, 1.0f} }, // Right
     { {-0.5f, -0.5f}, {0.0f, 0.0f, 1.0f, 1.0f}, {0.0f, 1.0f} }  // Left
    };
    MeshIndices indices = { 0, 1, 2 };

    _data->assets.AddMesh(StringId("tri"), verts, indices);

}

void StartState::Update(float dt)
{

    if (_data->renderer.StartRenderPass() != 0)
        return;

    MeshInstance meshInst{ StringId("tri") };
    MaterialInstance matInst{ StringId("tri_mat") };

    _data->renderer.DrawMesh(meshInst, matInst, { 0.0f, 0.0f }, { 1.0f, 1.0f }, 0.0f);

    _data->renderer.EndRenderPass();
    _data->renderer.Present();
}

