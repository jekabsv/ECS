#include "StartState.h"
#include "Level1.h"

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

    
    _data->assets.AddMesh("Triangle", triangleMesh);
    _data->assets.AddMesh("square", squareMesh);

    _data->assets.LoadBMPTexture("test", "../image.bmp", _data->SDLrenderer);
    _data->assets.LoadBMPTexture("player", "../player.bmp", _data->SDLrenderer);


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

    _data->state.AddState(StateRef(new Level1(_data)), 1);
}