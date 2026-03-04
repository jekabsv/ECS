#include "InputSystem.h"

using namespace InputSystem;
using namespace Internals;


std::shared_ptr<Device> Keyboards::LastUsedKeyboard = nullptr;

std::shared_ptr<Device> Mice::LastUsedMouse = nullptr;

std::shared_ptr<Device> Gamepads::LastUsedGamepad = nullptr;

std::vector<std::shared_ptr<Device>> Keyboards::keyboards;
std::vector<std::shared_ptr<Device>> Mice::mice;
std::vector<std::shared_ptr<Device>> Gamepads::gamepads;


void System::Init(float dt)
{
    // 2. Force SDL to "look" for hardware
    SDL_PumpEvents();

    // 3. Manually call Update once to drain the initial ADDED events
    // This populates Keyboards::All() via your switch(ev.type) logic
    Update(0.0f);

    // 4. Fallback: If SDL is being silent (common on laptops/headless)
    // and no devices were added via events, we force one.
    if (Keyboards::All().empty()) {
        auto fallback = std::make_shared<KeyboardDevice>(0);
        Keyboards::All().push_back(fallback);
        Keyboards::Current() = fallback;
    }

    if (Mice::All().empty()) {
        auto fallback = std::make_shared<MouseDevice>(0);
        Mice::All().push_back(fallback);
        Mice::Current() = fallback;
    }

    // 5. Standard Setup
    AddActionMap("default");
    AssignMapToPlayer(-1, "default");
    Update(dt);
}

ActionMap& System::GetActionMap(std::string ActionMapName)
{
    return ActionMaps[ActionMapName];
}
int System::RemoveActionMap(std::string ActionMapName)
{
    auto it = ActionMaps.find(ActionMapName);
    if (it == ActionMaps.end())
        return -1;
    ActionMaps.erase(it);
    return 0;
}
ActionState System::GetActionState(int player, std::string ActionName)
{
    return playerActionStates[player][ActionName].state;
}
ActionState System::GetActionState(std::string ActionName)
{
    return playerActionStates[-1][ActionName].state;
}
INPUT_DATA_4 System::GetActionAxis(int player, std::string ActionName)
{
    return playerActionStates[player][ActionName].data;
}
INPUT_DATA_4 System::GetActionAxis(std::string ActionName)
{
    return playerActionStates[-1][ActionName].data;
}
ActionMap& System::AddActionMap(std::string ActionMapName)
{
    return ActionMaps[ActionMapName];
}

int System::AssignMapToPlayer(int PlayerID, std::string Map)
{
    PlayerToMap[PlayerID] = Map;
    return 0;
}
int System::AssignDeviceToPlayer(int PlayerID, std::shared_ptr<Device> device)
{
    if (device == nullptr)
        return -1;
    DeviceType type = device->GetType();
    if (type == Keyboard) {
        PlayerKeyboardPool[PlayerID].push_back(device);
    }
    else if (type == Mouse) {
        PlayerMousePool[PlayerID].push_back(device);
    }
    else if (type == Gamepad) {
        PlayerGamepadPool[PlayerID].push_back(device);
    }
    return 0;
}
int System::AssignDevicesToPlayer(int PlayerID, std::vector<std::shared_ptr<Device>> devices)
{
    for (auto x : devices)
    {
        DeviceType type = x->GetType();
        if (type == Keyboard) {
            PlayerKeyboardPool[PlayerID].push_back(x);
        }
        else if (type == Mouse) {
            PlayerMousePool[PlayerID].push_back(x);
        }
        else if (type == Gamepad) {
            PlayerGamepadPool[PlayerID].push_back(x);
        }
    }
    return 0;
}
int System::RemoveDeviceFromPlayer(int playerID, std::shared_ptr<Device> device)
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

std::vector<std::shared_ptr<Device>> System::GetDevicesOfType(int player, DeviceType type)
{
    if (type == Keyboard)
        return PlayerKeyboardPool[player];
    if (type == Mouse)
        return PlayerMousePool[player];
    if (type == Gamepad)
        return PlayerGamepadPool[player];
    return {};
}

void System::Update(float dt)
{
    SDL_Event ev;
    int lastKeyboardID = -1;
    int lastMouseID = -1;
    int lastGamepadID = -1;

    while (SDL_PollEvent(&ev)) {
        switch (ev.type) {

        case SDL_EVENT_KEYBOARD_ADDED: {
            Keyboards::All().push_back(std::make_shared<KeyboardDevice>(ev.kdevice.which));
            break;
        }
        case SDL_EVENT_KEYBOARD_REMOVED: {
            auto id = ev.kdevice.which;
            std::erase_if(Keyboards::All(), [id](auto& d) { return d->GetId() == id; });
            break;
        }

        case SDL_EVENT_MOUSE_ADDED: {
            Mice::All().push_back(std::make_shared<MouseDevice>(ev.mdevice.which));
            break;
        }
        case SDL_EVENT_MOUSE_REMOVED: {
            auto id = ev.mdevice.which;
            std::erase_if(Mice::All(), [id](auto& d) { return d->GetId() == id; });
            break;
        }

        case SDL_EVENT_GAMEPAD_ADDED: {
            Gamepads::All().push_back(std::make_shared<GamepadDevice>(ev.gdevice.which));
            break;
        }
        case SDL_EVENT_GAMEPAD_REMOVED: {
            auto id = ev.gdevice.which;
            std::erase_if(Gamepads::All(), [id](auto& d) { return d->GetId() == id; });
            break;
        }


        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP: {
            lastKeyboardID = ev.key.which;
            bool isPressed = (ev.type == SDL_EVENT_KEY_DOWN);
            for (auto& dev : Keyboards::All()) {
                if (dev->GetId() == lastKeyboardID) {
                    static_cast<KeyboardDevice*>(dev.get())->SetState((int)ev.key.scancode, isPressed);
                    break;
                }
            }
            break;
        }

        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP: {
            lastMouseID = ev.button.which;
            bool isPressed = (ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
            for (auto& dev : Mice::All()) {
                if (dev->GetId() == lastMouseID) {
                    static_cast<MouseDevice*>(dev.get())->SetButton((int)ev.button.button, isPressed);
                    break;
                }
            }
            break;
        }

        case SDL_EVENT_MOUSE_MOTION: {
            lastMouseID = ev.motion.which;
            for (auto& dev : Mice::All()) {
                if (dev->GetId() == lastMouseID) {
                    auto m = static_cast<MouseDevice*>(dev.get());
                    m->SetAxis(0, ev.motion.x);
                    m->SetAxis(1, ev.motion.y);
                    break;
                }
            }
            break;
        }

        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        case SDL_EVENT_GAMEPAD_BUTTON_UP: {
            lastGamepadID = ev.gbutton.which;
            bool isPressed = (ev.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
            for (auto& dev : Gamepads::All()) {
                if (dev->GetId() == lastGamepadID) {
                    static_cast<GamepadDevice*>(dev.get())->SetButton((int)ev.gbutton.button, isPressed);
                    break;
                }
            }
            break;
        }

        case SDL_EVENT_GAMEPAD_AXIS_MOTION: {
            lastGamepadID = ev.gaxis.which;
            for (auto& dev : Gamepads::All()) {
                if (dev->GetId() == lastGamepadID) {
                    float normalized = ev.gaxis.value / 32767.0f;
                    static_cast<GamepadDevice*>(dev.get())->SetAxis((int)ev.gaxis.axis, normalized);
                    break;
                }
            }
            break;
        }
        }
    }

    auto UpdateCurrent = [](auto& allList, auto& currentPtr, int lastID) {
        if (lastID != -1) {
            for (auto& dev : allList) {
                if (dev->GetId() == lastID) {
                    currentPtr = dev;
                    break;
                }
            }
        }
        if (allList.empty()) currentPtr = nullptr;
        };

    UpdateCurrent(Keyboards::All(), Keyboards::Current(), lastKeyboardID);
    UpdateCurrent(Mice::All(), Mice::Current(), lastMouseID);
    UpdateCurrent(Gamepads::All(), Gamepads::Current(), lastGamepadID);

    auto PurgePool = [](auto& playerPoolMap, const auto& activeList) {
        for (auto& [playerId, devices] : playerPoolMap) {
            std::erase_if(devices, [&activeList](const std::shared_ptr<Device>& dev) {
                return std::find(activeList.begin(), activeList.end(), dev) == activeList.end();
                });
        }
        };

    PurgePool(PlayerKeyboardPool, Keyboards::All());
    PurgePool(PlayerMousePool, Mice::All());
    PurgePool(PlayerGamepadPool, Gamepads::All());

    for (auto& [playerId, mapName] : PlayerToMap)
    {
        playerActionStates[playerId] = ActionMaps[mapName].GetAllActionStates(
            dt,
            playerId,
            playerActionStates[playerId],
            PlayerKeyboardPool[playerId],
            PlayerMousePool[playerId],
            PlayerGamepadPool[playerId]
        );
    }
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

std::unordered_map<std::string, ActionStateClass> ActionMap::GetAllActionStates(float dt, int player,
    std::unordered_map<std::string, ActionStateClass> previousActionState,
    std::vector<std::shared_ptr<Device>>& PlayerKeyboardPool, std::vector<std::shared_ptr<Device>>& PlayerMousePool,
    std::vector<std::shared_ptr<Device>>& PlayerGamepadPool)
{
    std::unordered_map<std::string, ActionStateClass> actionmap;
    for (auto& [name, action] : Actions)
        actionmap[name] = action.GetActionState(dt, player, previousActionState[name], PlayerKeyboardPool, PlayerMousePool, PlayerGamepadPool);
    return actionmap;
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

ActionStateClass Action::GetActionState(float dt, int player, ActionStateClass previousState, std::vector<std::shared_ptr<Device>> PlayerKeyboardPool,
    std::vector<std::shared_ptr<Device>> PlayerMousePool, std::vector<std::shared_ptr<Device>> PlayerGamepadPool)
{
    if (!active)
        return ActionStateClass{ ActionState::Idle, INPUT_DATA_4{{0, 0, 0, 0}} };

    INPUT_DATA_4 rawData = { { 0.0f, 0.0f, 0.0f, 0.0f } };
    bool isAnyBindingPressed = false;

    for (const auto& binding : bindings)
    {
        const auto* targetPool = &PlayerKeyboardPool;
        if (binding.device == DeviceType::Mouse)
            targetPool = &PlayerMousePool;
        else if (binding.device == DeviceType::Gamepad)
            targetPool = &PlayerGamepadPool;

        for (auto& device : *targetPool) {
            float val = 0.0f;
            if (binding.bindingType == BindingType::Button)
            {
                val = device->IsPressed(binding.key);
            }
            else
            {
                val = device->GetAxis(binding.key);
            }

            if (val != 0.0f) {
                rawData[binding.componentIndex] += val * binding.scale;
                isAnyBindingPressed = true;
            }
        }
    }

    INPUT_DATA_4 processedData = rawData;

    for (auto& processor : processors) {
        processor->Process(processedData);
    }

    ActionState currentRawState = ActionState::Idle;
    if (isAnyBindingPressed)
    {
        if (previousState.state == ActionState::Held)
            currentRawState = ActionState::Held;
        else if (previousState.state == ActionState::Pressed)
            currentRawState = ActionState::Held;
        else
            currentRawState = ActionState::Pressed;
    }
    else
    {
        if (previousState.state == ActionState::Relesased)
            currentRawState = ActionState::Idle;
        else if (previousState.state == ActionState::Idle)
            currentRawState = ActionState::Idle;
        else
            currentRawState = ActionState::Relesased;
    }

    ActionState finalState = currentRawState;

    for (auto& interaction : interactions)
    {
        interaction->Update(processedData, dt, currentRawState);
        interaction->TriggerEvents(processedData);
    }

    return ActionStateClass{ finalState, processedData };
}

void Action::Activate()
{
    active = true;
}
void Action::Deactivate()
{
    active = false;
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
std::shared_ptr<Device>& Gamepads::Current()
{
    return LastUsedGamepad;
}


std::vector<std::shared_ptr<Device>>& Keyboards::All()
{
    return keyboards;
}
std::shared_ptr<Device>& Keyboards::Current()
{
    return LastUsedKeyboard;
}


std::vector<std::shared_ptr<Device>>& Mice::All()
{
    return mice;
}
std::shared_ptr<Device>& Mice::Current()
{
    return LastUsedMouse;
}
