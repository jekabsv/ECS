#include <iostream>
#include <format>

#include "Uicontext.h";
#include "StartState.h"
#include "Level1.h"
#include "SPH.h"
#include "Boids.h"
#include "Heat.h"
#include "NBody.h"


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

static bool focused;

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
	_data->assets.AddTexture("test", _data->renderer.CreateTexture(_data->assets.TryGetSurface("test")));

    _data->assets.LoadBMPSurface("player", "../ECS/player.bmp");
    _data->assets.AddTexture("player", _data->renderer.CreateTexture(_data->assets.TryGetSurface("player")));

    _data->assets.LoadBMPSurface("demo", "../ECS/demo.bmp");
    _data->assets.AddTexture("demo", _data->renderer.CreateTexture(_data->assets.TryGetSurface("demo")));

    _data->assets.LoadFont("tnr", "../ECS/times.ttf");
    _data->assets.AddGPUFont("tnr", _data->renderer.CreateFontt("tnr"));

	_data->assets.AddTexture("atlas", _data->assets.TryGetGPUFont("tnr")->atlas);



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



    //Meshes
    MeshVertices verts = {
    {  0.0f, -100.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.5f, 0.0f} ,
    {  100.0f, 100.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f, 1.0f} ,
    { -100.0f, 100.0f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f} 
    };
    MeshIndices indices = { 0, 1, 2 };

    _data->assets.AddMesh(StringId("tri"), verts, indices);



    _data->animation.AddClip("player_idle_right", { "player", 64, 64, 192, 1, 1, 1, 0.1f });
    _data->animation.AddClip("player_idle_left", { "player", 64, 64, 192, 3, 1, 1, 0.1f });

    _data->animation.AddClip("player_run_right", { "player", 64, 64, 0, 0, 8, 8, 0.08f });
    _data->animation.AddClip("player_run_left", { "player", 64, 64, 0, 2, 8, 8, 0.08f });

    _data->animation.AddClip("demo", {
    .spritesheet = "demo",
    .frameWidth = 380,
    .frameHeight = 240,
    .startX = 0,
    .startRow = 0,
    .totalFrames = 326,   // 19*17 + 3
    .columns = 19,
    .frameDuration = 0.16666f/2.f });

    _data->animation.Play(anim, "demo");


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




    //UI
    ui.GetTheme().LoadDarkDefaults();
    ui.GetTheme().SetToken("font-default", StringId("tnr"));


    UI::NodeHandle root = ui.AddContainer();
    ui.SetSize(root, UI::SizeValue::Auto(), UI::SizeValue::Auto());
	ui.SetFlexDirection(root, UI::FlexDirection::Row);

    UI::NodeHandle left = ui.AddContainer(root);
	ui.SetSize(left, UI::SizeValue::Px(300), UI::SizeValue::Auto());

	UI::NodeHandle right = ui.AddContainer(root);
	ui.SetSize(right, UI::SizeValue::Auto(), UI::SizeValue::Auto());
	//ui.SetFlexBasis(right, UI::SizeValue::Percent(100));
	ui.SetJustify(right, UI::JustifyContent::Center);

    ui.SetFlexDirection(left, UI::FlexDirection::Column);
    ui.SetJustify(left, UI::JustifyContent::Center);
	ui.SetPadding(left, UI::Edges::All(20.f));


    ui.SetGap(left, 16.f);
    auto img = ui.AddImage("player", { 0, 0, 64, 64 }, left);
    ui.SetSize(img, UI::SizeValue::Px(128), UI::SizeValue::Px(192));

    btnSillyGame = ui.AddButton("Test world", left);
    btnBoids = ui.AddButton("Boids", left);
    btnSPH = ui.AddButton("SPH", left);
    btnHeat = ui.AddButton("Heat", left);
    btnNBody = ui.AddButton("N-Body", left);

    tickRateLabel = ui.AddLabel("target tick rate: (60)", left);
    tickRateSlider = ui.AddSlider(60.f, 30.f, 120.f, left);
    btnQuit_ = ui.AddButton("Quit", left);
    
}

void StartState::Update(float dt) 
{

    auto x = ui.GetSliderValue(tickRateSlider);
	_data->TargetTickRate = (int)x;
	ui.SetText(tickRateLabel, std::format("target tick rate: ({:.0f})", x));


    if (ui.IsClicked(btnSillyGame))
		_data->state.AddState(StateRef(new Level1(_data)), 0);
    if (ui.IsClicked(btnBoids))
        _data->state.AddState(StateRef(new Boids(_data)), 0);
    if (ui.IsClicked(btnSPH))
		_data->state.AddState(StateRef(new SPH(_data)), 0);
    if (ui.IsClicked(btnHeat))
        _data->state.AddState(StateRef(new Heat(_data)), 0);
    if (ui.IsClicked(btnNBody))
        _data->state.AddState(StateRef(new NBody(_data)), 0);
    if (ui.IsClicked(btnQuit_))
        _data->quit = true;
}

void StartState::Render(float dt)
{

	//ui.SetImage(demoImage, anim.currentSpritesheet, anim.currentRect);
	//_data->renderer.SubmitSprite(MaterialInstance("sprite_mat", anim.currentSpritesheet), anim.currentRect, { 400.f, 300.f }, { 1.f, 1.f }, 0.0f, { 1.f, 1.f, 1.f, 1.f });
}
