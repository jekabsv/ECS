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

    //Assets
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

    _data->assets.AddMesh("Triangle", triangleMesh);
    _data->assets.AddMesh("square", squareMesh);

    _data->assets.LoadBMPTexture("test", "../image.bmp", _data->SDLrenderer);
    _data->assets.LoadBMPTexture("player", "../player.bmp", _data->SDLrenderer);


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


    //Animations
    _data->animation.AddClip("player_idle_right", { "player", 64, 64, 192, 192, 1, 0.1f });
    _data->animation.AddClip("player_idle_left", { "player", 64, 64, 192, 192, 3, 0.1f });
    _data->animation.AddClip("player_run_right", { "player", 64, 64, 0,   448, 0, 0.08f });
    _data->animation.AddClip("player_run_left", { "player", 64, 64, 0,   448, 2, 0.08f });


    _data->assets.LoadFont("main", "../OpenSans-Regular.ttf");


    printf("Font registered: %p\n", _data->assets.GetFont("main"));


    ui.GetTheme().LoadDarkDefaults();

    ui.RegisterFont("main", _data->assets.GetFont("main"));

    UI::NodeHandle root = ui.AddContainer();
    ui.SetSize(root, UI::SizeValue::Px(500), UI::SizeValue::Auto());
    ui.SetFlexDirection(root, UI::FlexDirection::Column);
    ui.SetJustify(root, UI::JustifyContent::Center);


    ui.SetAlignItems(root, UI::AlignItems::Stretch);
    ui.SetGap(root, 12.0f);
    ui.SetPadding(root, UI::Edges::All(20.0f));
    

    ui.AddImage(_data->assets.GetTexture("player"), { 0, 0, 64, 64 }, root);
    btnSillyGame = ui.AddButton("Silly game", root);
    btnBoids = ui.AddButton("Boids", root);
    btnQuit_ = ui.AddButton("Quit", root);
	btnSlider = ui.AddSlider(0.5f, 0.0f, 1.0f, root);
	ui.AddLabel("This is a label", root);
	ui.AddInputField("Type here...", root);
}

void StartState::Update(float dt)
{
	if(ui.IsClicked(btnSillyGame))
    {
		ui.ClearChildren(0);
        _data->state.AddState(StateRef(new Level1(_data)), 0);
    }
    else if(ui.IsClicked(btnBoids))
    {
        ui.ClearChildren(0);
        _data->state.AddState(StateRef(new Boids(_data)), 0);
    }
    else if(ui.IsClicked(btnQuit_))
    {
        _data->quit = true;
	}
}

