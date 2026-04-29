#include "StartState.h"
#include "Level1.h"
#include <windows.h>
//#include "SPH.h"
#include "Boids.h"
#include "Uicontext.h";
#include <iostream>
#include "depthState.h"

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
    _data->assets.LoadShader(StringId("vertSprite"), "../ECS/vertSpriteInstancing.spv", _data->device, ShaderStage::VERTEX,
        /*samplers*/0, /*storageTex*/0, /*storageBuf*/1 , /*uniformBufs*/1);

    _data->assets.LoadShader(StringId("fragSprite"), "../ECS/fragSpriteInstancing.spv", _data->device, ShaderStage::FRAGMENT,
        /*samplers*/1, /*storageTex*/0, /*storageBuf*/0, /*uniformBufs*/0);


    _data->assets.LoadShader(StringId("vert"), "../ECS/vertInstancing.spv", _data->device, ShaderStage::VERTEX,
        /*samplers*/0, /*storageTex*/0, /*storageBuf*/1, /*uniformBufs*/1);

    _data->assets.LoadShader(StringId("frag"), "../ECS/fragInstancing.spv", _data->device, ShaderStage::FRAGMENT,
        /*samplers*/0, /*storageTex*/0, /*storageBuf*/0, /*uniformBufs*/0);






	//Textures
    _data->assets.LoadBMPSurface("test", "../ECS/test.bmp");
	_data->assets.AddTexture("test", _data->renderer.CreateTexture(_data->assets.GetSurface("test")));

    _data->assets.LoadBMPSurface("player", "../ECS/player.bmp");
    _data->assets.AddTexture("player", _data->renderer.CreateTexture(_data->assets.GetSurface("player")));


    _data->assets.LoadFont("tnr", "../ECS/times.ttf");
    _data->assets.AddGPUFont("tnr", _data->renderer.CreateFontt("tnr"));


 



    //Materials
    MaterialBase spriteMaterial("vertSprite", "fragSprite");
    MaterialBase::MakeOpaque(spriteMaterial);
    MaterialBase::SetVertexAttr(spriteMaterial);

    _data->assets.AddMaterial(StringId("sprite_mat"), spriteMaterial);



    MaterialBase mat("vert", "frag");
    MaterialBase::MakeOpaque(mat);
    MaterialBase::SetVertexAttr(mat);

    _data->assets.AddMaterial(StringId("mat"), mat);


    MaterialBase::MakeAdditive(mat);

    _data->assets.AddMaterial(StringId("mat_transp"), mat);
    


    MaterialBase textMat("vertSprite", "fragSprite");
    MaterialBase::MakeOverlay(textMat);
    MaterialBase::SetVertexAttr(textMat);
    _data->assets.AddMaterial(StringId("text_mat"), textMat);


    MaterialBase uiMat("vert", "frag");
    MaterialBase::MakeOverlay(uiMat);
    MaterialBase::SetVertexAttr(uiMat);

    _data->assets.AddMaterial(StringId("ui_mat"), uiMat);




    //Meshes
    MeshVertices verts = {
    {  0.0f, -100.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.0f} ,
    {  100.0f, 100.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f} ,
    { -100.0f, 100.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f} 
    };
    MeshIndices indices = { 0, 1, 2 };

    _data->assets.AddMesh(StringId("tri"), verts, indices);

    //Animations
    _data->animation.AddClip("player_idle_right", { "player", 64, 64, 192, 192, 1, 0.1f });
    _data->animation.AddClip("player_idle_left", { "player", 64, 64, 192, 192, 3, 0.1f });
    _data->animation.AddClip("player_run_right", { "player", 64, 64, 0,   448, 0, 0.08f });
    _data->animation.AddClip("player_run_left", { "player", 64, 64, 0,   448, 2, 0.08f });


    //Inputs
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

    _data->inputs.GetActionMap("level1")->AddAction("next")
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_SPACE);

    _data->inputs.GetActionMap("level1")->AddAction("show_colliders")
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_LSHIFT);

    _data->inputs.AssignDeviceToPlayer(InputSystem::KeyboardHub::Current());

    _data->inputs.GetActionMap("level1")->AddAction("scale")
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_Q, 0)
        .AddBinding(InputSystem::Button, InputSystem::Keyboard, SDL_SCANCODE_E, 1)
        .AddProcessor(std::make_unique<ProcessQE>("qe"));

    _data->inputs.GetActionMap("level1")->AddAction("click")
        .AddBinding(InputSystem::Button, InputSystem::Mouse, SDL_BUTTON_LEFT);
    _data->inputs.GetActionMap("level1")->AddAction("mousePos")
        .AddBinding(InputSystem::Axis, InputSystem::Mouse, 0, 0)
        .AddBinding(InputSystem::Axis, InputSystem::Mouse, 1, 1);

    _data->inputs.AssignMapToPlayer("level1");

    //_data->state.AddState(StateRef(new depthState(_data)), 0);
    //_data->state.AddState(StateRef(new Level1(_data)), 0);
    //_data->state.AddState(StateRef(new Boids(_data)), 0);


    ui.RegisterFont("font-default", _data->assets.GetFont(StringId("tnr")), StringId("tnr"));

    UI::NodeHandle panel = ui.AddContainer();
    ui.SetSize(panel, UI::SizeValue::Px(300.0f), UI::SizeValue::Px(200.0f));
    ui.SetFlexDirection(panel, UI::FlexDirection::Column);
    ui.SetAlignItems(panel, UI::AlignItems::Stretch);
    ui.SetGap(panel, 12.0f);
    ui.SetPadding(panel, UI::Edges::All(16.0f));
    ui.SetMargin(panel, UI::Edges::TRBL(200.0f, 0.0f, 0.0f, 200.0f));

    UI::StyleOverride bg;
    bg.background = UI::Color::RGBA(40, 40, 50, 220);
    ui.SetStyleOverride(panel, bg);

    btnTest = ui.AddButton("Click Me", panel);
    sliderVol = ui.AddSlider(0.5f, 0.0f, 1.0f, panel);
}

float rotation = 0;

void StartState::Update(float dt) {
    rotation += _data->inputs.GetActionAxis("move")[0] * dt * 5;
}

void StartState::Render(float dt)
{
    _data->renderer.SubmitText("hello world!", "tnr", MaterialInstance("text_mat"), { 100.f, 150.f, 0.f }, { 1.f, 1.f }, rotation);

    auto gpuFont = _data->assets.GetGPUFont("tnr");

    SDL_FRect fullRect = { 0, 0, (float)gpuFont->atlas.width, (float)gpuFont->atlas.height };
}

