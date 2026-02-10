#include "InputSystem.h"
#include "Devices.h"
#include <algorithm>

// backend includes here (SDL, etc.)

void InputSystem::Init()
{
    int kbcount = 0;
    auto keyboards = SDL_GetKeyboards(&kbcount);
    int mcount = 0;
    auto mouse = SDL_GetMice(&mcount);
    int jcount = 0;
    auto joysticks = SDL_GetJoysticks(&jcount);
    int gcount = 0;
    auto gamepads = SDL_GetGamepads(&gcount);

    for(int i = 0;i < kbcount;i++)
        Keyboard::Devices.push_back(InputDevice())


    Keyboard::CurrentDevice = kb;

    auto* mouse = new MouseDeviceSDL();
    Mouse::Devices.push_back(mouse);
    Mouse::CurrentDevice = mouse;

    // enumerate gamepads
    int count = SDL_NumJoysticks();
    for (int i = 0; i < count; ++i)
        if (SDL_IsGamepad(i))
            OnGamepadConnected(i);
}

ActionMap& InputSystem::CreateActionMap(const std::string& name)
{
    return ActionMaps[name];
}

void InputSystem::DeleteActionMap(const std::string& name)
{
    ActionMaps.erase(name);
}

void InputSystem::SubscribeUser(UserID user, const std::string& map)
{
    Users[user].ActionMap = map;
}

void InputSystem::AssignDevice(UserID user, InputDevice& device)
{
    auto& v = Users[user].Devices;
    if (std::find(v.begin(), v.end(), &device) == v.end())
        v.push_back(&device);
}

void InputSystem::UnassignDevice(InputDevice& device)
{
    for (auto& [id, ctx] : Users)
    {
        auto& v = ctx.Devices;
        v.erase(std::remove(v.begin(), v.end(), &device), v.end());
    }
}

void InputSystem::OnGamepadConnected(int index)
{
    auto* pad = new GamepadDeviceSDL(index);
    Gamepad::Devices.push_back(pad);
    Gamepad::CurrentDevice = pad;
}

void InputSystem::OnGamepadDisconnected(int instanceID)
{
    auto& v = Gamepad::Devices;

    auto it = std::find_if(v.begin(), v.end(),
        [&](InputDevice* d)
        {
            return static_cast<GamepadDeviceSDL*>(d)->InstanceID() == instanceID;
        });

    if (it == v.end())
        return;

    InputDevice* dead = *it;

    UnassignDevice(*dead);

    if (Gamepad::CurrentDevice == dead)
        Gamepad::CurrentDevice = nullptr;

    delete dead;
    v.erase(it);

    if (!Gamepad::Devices.empty())
        Gamepad::CurrentDevice = Gamepad::Devices[0];
}

void InputSystem::Update()
{
    auto pollGroup = [](auto& devices, InputDevice*& current)
        {
            for (auto* d : devices)
                d->BeginFrame();

            for (auto* d : devices)
            {
                d->Poll();
                if (d->IsActiveThisFrame())
                    current = d;
            }
        };

    pollGroup(Keyboard::Devices, Keyboard::CurrentDevice);
    pollGroup(Mouse::Devices, Mouse::CurrentDevice);
    pollGroup(Gamepad::Devices, Gamepad::CurrentDevice);
}

ActionValue InputSystem::GetAction(UserID user, const std::string& action) const
{
    ActionValue out{};

    const auto& ctx = Users.at(user);
    const auto& map = ActionMaps.at(ctx.ActionMap);
    const auto& act = map.Actions.at(action);

    if (!map.Enabled || !act.Enabled)
        return out;

    for (const auto& bind : act.Bindings)
    {
        for (auto* dev : ctx.Devices)
        {
            if (dev->GetType() != bind.device)
                continue;

            if (dev->GetButtonState(bind.code) == ButtonState::Pressed)
                out.axes[0] = 1.0f;
        }
    }

    for (const auto& p : act.Processors)
        p->Process(out);

    return out;
}
