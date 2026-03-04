#include "InputSystem.h"


std::shared_ptr<Device> Keyboards::LastUsedKeyboard = nullptr;
static std::vector<std::shared_ptr<Device>> g_Keyboards;
std::vector<std::shared_ptr<Device>> Keyboards::keyboards = g_Keyboards;

std::shared_ptr<Device> Mice::LastUsedMouse = nullptr;
static std::vector<std::shared_ptr<Device>> g_Mice;
std::vector<std::shared_ptr<Device>> Mice::mice = g_Mice;

std::shared_ptr<Device> Gamepads::LastUsedGamepad = nullptr;
static std::vector<std::shared_ptr<Device>> g_Gamepads;
std::vector<std::shared_ptr<Device>> Gamepads::gamepads = g_Gamepads;


ActionMap& InputSystem::GetActionMap(std::string ActionMapName)
{
    return ActionMaps[ActionMapName];
}
int InputSystem::RemoveActionMap(std::string ActionMapName)
{
    auto it = ActionMaps.find(ActionMapName);
    if (it == ActionMaps.end())
        return -1;
    ActionMaps.erase(it);
    return 0;
}
OptionalRef<ActionMap> InputSystem::AddActionMap(std::string ActionMapName)
{
    return std::ref(ActionMaps[ActionMapName]);
}

ActionState InputSystem::GetActionState(int player, std::string ActionName)
{
    auto playerMapIt = PlayerToMap.find(player);
    if (playerMapIt == PlayerToMap.end())
        return ActionState::Error;

    const std::string& mapName = playerMapIt->second;

    auto mapIt = ActionMaps.find(mapName);
    if (mapIt == ActionMaps.end())
        return ActionState::Error;

    return mapIt->second.GetActionState(ActionName);
}
ActionState InputSystem::GetActionState(std::string ActionName)
{
    return defaultActionMap.GetActionState(ActionName);
}
INPUT_DATA_4 InputSystem::GetActionAxis(int player, std::string ActionName)
{
    auto playerMapIt = PlayerToMap.find(player);
    if (playerMapIt == PlayerToMap.end())
        return INPUT_DATA_4{ 0, 0, 0, 0 };

    const std::string& mapName = playerMapIt->second;

    auto mapIt = ActionMaps.find(mapName);
    if (mapIt == ActionMaps.end())
        return INPUT_DATA_4{ 0, 0, 0, 0 };

    return mapIt->second.GetActionAxis(ActionName);
}
INPUT_DATA_4 InputSystem::GetActionAxis(std::string ActionName)
{
    return defaultActionMap.GetActionAxis(ActionName);
}

int InputSystem::AssignMapToPlayer(int PlayerID, std::string Map)
{
    PlayerToMap[PlayerID] = Map;
    return 0;
}
int InputSystem::AssignDeviceToPlayer(int PlayerID, std::shared_ptr<Device> device)
{
    DeviceType type = device->GetType();
    if (type == Keyboard) {
        PlayerKeyboardPool[PlayerID].push_back(device);
    }
    return 0;
}
int InputSystem::RemoveDeviceFromPlayer(int playerID, std::shared_ptr<Device> device)
{
    if (!device)
        return -1;

    DeviceType type = device->GetType();
    std::vector<std::shared_ptr<Device>>* targetPool = nullptr;

    if (type == Keyboard)
    {
        targetPool = &PlayerKeyboardPool[playerID];
    }
    else if (type == Mouse)
    {
        targetPool = &PlayerMousePool[playerID];
    }
    else if (type == Gamepad)
    {
        targetPool = &PlayerGamepadPool[playerID];
    }

    if (targetPool)
    {
        auto it = std::find(targetPool->begin(), targetPool->end(), device);
        if (it != targetPool->end())
        {
            targetPool->erase(it);
            return 0;
        }
    }

    return -1;
}

std::vector<std::shared_ptr<Device>> InputSystem::GetDevicesOfType(int player, DeviceType type)
{
    if (type == Keyboard)
        return PlayerKeyboardPool[player];
    if (type == Mouse)
        return PlayerMousePool[player];
    if (type == Gamepad)
        return PlayerGamepadPool[player];
    return std::vector<std::shared_ptr<Device>>{NULL};
}

void InputSystem::Update(int dt)
{
    //Update Active keyboards
    int count;
    auto kb = SDL_GetKeyboards(&count);
    Keyboards::All().clear();
    if (kb)
    {
        for (int i = 0; i < count; ++i) {
            Keyboards::All().push_back(std::make_shared<KeyboardDevice>(kb[i]));
        }
        SDL_free(kb);
    }

    //Update Active Mice
    auto m = SDL_GetMice(&count);
    Mice::All().clear();
    if (m)
    {
        for (int i = 0; i < count; ++i) {
            Mice::All().push_back(std::make_shared<MouseDevice>(m[i]));
        }
        SDL_free(m);
    }

    //Update Active Gamepads
    auto gp = SDL_GetGamepads(&count);
    Gamepads::All().clear();
    if (gp)
    {
        for (int i = 0; i < count; ++i) {
            Gamepads::All().push_back(std::make_shared<GamepadDevice>(gp[i]));
        }
        SDL_free(gp);
    }

    //Update Current device & get input states
    SDL_Event ev;
    int lastMouseID;
    int lastKeyboardID;
    int lastGamepadID;
    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {
            // keyboard
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
        {
            lastKeyboardID = ev.key.which;
            bool isPressed = (ev.type == SDL_EVENT_KEY_DOWN);
            for (auto& dev : Keyboards::All())
            {
                if (dev->GetId() == lastKeyboardID) {
                    auto kb = std::static_pointer_cast<KeyboardDevice>(dev);
                    kb->SetState((int)ev.key.scancode, isPressed);
                    break;
                }
            }
            break;
        }

        // mouse
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
        {
            lastMouseID = ev.button.which;
            bool isPressed = (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
            for (auto& dev : Mice::All()) {
                if (dev->GetId() == lastMouseID) {
                    auto mouse = std::static_pointer_cast<MouseDevice>(dev);
                    mouse->SetButton((int)ev.button.button, isPressed);
                    break;
                }
            }
            break;
        }

        // mouse motion & wheel
        case SDL_EVENT_MOUSE_MOTION:
        {
            lastMouseID = ev.motion.which;
            for (auto& dev : Mice::All())
            {
                if (dev->GetId() == lastMouseID)
                {
                    auto mouse = std::static_pointer_cast<MouseDevice>(dev);
                    mouse->SetAxis(0, ev.motion.x);
                    mouse->SetAxis(1, ev.motion.y);
                    break;
                }
            }
            break;
        }
        case SDL_EVENT_MOUSE_WHEEL:
        {
            lastMouseID = ev.wheel.which;
            for (auto& dev : Mice::All())
            {
                if (dev->GetId() == lastMouseID)
                {
                    auto mouse = std::static_pointer_cast<MouseDevice>(dev);
                    mouse->SetAxis(2, ev.wheel.y);
                    mouse->SetAxis(3, ev.wheel.x);
                    break;
                }
            }
            break;
        }

        // gamepad buttons
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
        {
            lastGamepadID = ev.gbutton.which;
            bool isPressed = (ev.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
            for (auto& dev : Gamepads::All())
            {
                if (dev->GetId() == lastGamepadID)
                {
                    auto pad = std::static_pointer_cast<GamepadDevice>(dev);
                    pad->SetButton((int)ev.gbutton.button, isPressed);
                    break;
                }
            }
            break;
        }

        // gamepad axis
        case SDL_EVENT_GAMEPAD_AXIS_MOTION:
        {
            lastGamepadID = ev.gaxis.which;
            for (auto& dev : Gamepads::All())
            {
                if (dev->GetId() == lastGamepadID)
                {
                    auto pad = std::static_pointer_cast<GamepadDevice>(dev);
                    float normalized = ev.gaxis.value / 32767.0f;
                    pad->SetAxis((int)ev.gaxis.axis, normalized);
                    break;
                }
            }
            break;
        }
        }
    }

    for (auto& x : Keyboards::All()) {
        if (x->GetId() == lastKeyboardID) {
            Keyboards::Current() = x;
            break;
        }
    }
    if (Keyboards::All().empty())
        Keyboards::Current;

    for (auto& x : Mice::All()) {
        if (x->GetId() == lastMouseID) {
            Mice::Current() = x;
            break;
        }
    }
    if (Mice::All().empty())
        Mice::Current;

    for (auto& x : Gamepads::All()) {
        if (x->GetId() == lastGamepadID) {
            Gamepads::Current() = x;
            break;
        }
    }
    if (Gamepads::All().empty())
        Gamepads::Current;


    auto PurgePool = [](auto& pool, const std::vector<std::shared_ptr<Device>>& activeList)
        {
            for (auto& [playerId, devices] : pool) {
                devices.erase(std::remove_if(devices.begin(), devices.end(),
                    [&activeList](const std::shared_ptr<Device>& dev) {
                        return std::find(activeList.begin(), activeList.end(), dev) == activeList.end();
                    }), devices.end());
            }
        };

    PurgePool(PlayerKeyboardPool, Keyboards::All());
    PurgePool(PlayerMousePool, Mice::All());
    PurgePool(PlayerGamepadPool, Gamepads::All());

    for (auto& [name, map] : ActionMaps)
    {
        if (map.IsActive())
            map.Update(dt);
    }
}



void ActionMap::Update(int dt)
{
    for (auto& [name, action] : Actions)
        if (action.IsActive())
            action.Update(dt);
}

void ActionMap::Activate()
{
    active = true;
}
void ActionMap::Deactivate()
{
    active = false;
}
bool ActionMap::IsActive()
{
    return active;
}

ActionState ActionMap::GetActionState(std::string ActionName)
{
    return Actions[ActionName].GetState();
}
INPUT_DATA_4 ActionMap::GetActionAxis(std::string ActionName)
{
    return Actions[ActionName].GetActionAxis();
}


ActionState Action::GetState()
{
    return state;
}
INPUT_DATA_4 Action::GetActionAxis()
{
    return data;
}
Action& ActionMap::AddAction(std::string name)
{
    return Actions[name];
}
OptionalRef<Action> ActionMap::GetAction(std::string name)
{
    auto it = Actions.find(name);
    if (it == Actions.end())
        return std::nullopt;
    return it->second;

}
int ActionMap::RemoveAction(std::string name)
{
    auto it = Actions.find(name);
    if (it == Actions.end())
        return -1;
    Actions.erase(name);
    return 0;
}

Action& Action::AddBinding(Bindings binding)
{
    bindings.push_back(binding);
    return *this;
}
Action& Action::AddProcessor(std::unique_ptr<Processor> processor)
{
    processors.push_back(std::move(processor));
    return *this;
}
Action& Action::AddInteraction(std::unique_ptr<Interaction> interaction)
{
    interactions.push_back(std::move(interaction));
    return *this;
}
int Action::RemoveBinding(Bindings binding)
{
    for (auto it = bindings.begin(); it < bindings.end(); ++it)
        if (*it == binding)
        {
            std::swap(bindings.back(), *it);
            bindings.pop_back();
            return 0;
        }
    return -1;
}
int Action::RemoveProcessor(std::string name)
{
    for (auto it = processors.begin(); it < processors.end(); ++it)
        if ((*it)->name == name)
        {
            std::swap(processors.back(), *it);
            processors.pop_back();
            return 0;
        }
    return -1;
}
int Action::RemoveInteraction(std::string name)
{
    for (auto it = interactions.begin(); it < interactions.end(); ++it)
        if ((*it)->name == name)
        {
            std::swap(interactions.back(), *it);
            interactions.pop_back();
            return 0;
        }
    return -1;
}

bool Action::IsActive()
{
    return active;
}
void Action::Activate()
{
    active = true;
}
void Action::Deactivate()
{
    active = false;
}

void Action::Update(int dt)
{
    for (auto& x : interactions)
        x->Update(data, dt, state);

    for (auto& x : interactions)
        x->TriggerEvents(data);

    return;
}


DeviceType Device::GetType()
{
    return type;
}
int Device::GetId()
{
    return DeviceId;
}


std::vector<std::shared_ptr<Device>>& Gamepads::All()
{
    return gamepads;
}
std::shared_ptr<Device> Gamepads::Current()
{
    return LastUsedGamepad;
}


std::vector<std::shared_ptr<Device>>& Keyboards::All()
{
    return keyboards;
}
std::shared_ptr<Device> Keyboards::Current()
{
    return LastUsedKeyboard;
}


std::vector<std::shared_ptr<Device>>& Mice::All()
{
    return mice;
}
std::shared_ptr<Device> Mice::Current()
{
    return LastUsedMouse;
}
