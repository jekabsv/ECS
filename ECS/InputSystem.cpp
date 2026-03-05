#include "InputSystem.h"

using namespace InputSystem;
using namespace Internals;

std::shared_ptr<Device> KeyboardHub::keyboard = nullptr;
std::shared_ptr<Device> MouseHub::mouse = nullptr;

std::shared_ptr<Device> GamepadHub::LastUsedGamepad = nullptr;
std::vector<std::shared_ptr<Device>> GamepadHub::gamepads;

void System::Init()
{
	SDL_PumpEvents();
	Update(0.0f);

	if (!KeyboardHub::Current()) {
		KeyboardHub::Current() = std::make_shared<KeyboardDevice>(0);
	}
	if (!MouseHub::Current()) {
		MouseHub::Current() = std::make_shared<MouseDevice>(0);
	}

	AddActionMap("default");
	AssignMapToPlayer(-1, "default");
	Update(0.0f);
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
ActionState System::GetActionState(std::string ActionName, int player)
{
	return playerActionStates[player][ActionName].state;
}
INPUT_DATA_4 System::GetActionAxis(std::string ActionName, int player)
{
	return playerActionStates[player][ActionName].data;
}

bool System::IsHeld(std::string ActionName, int player)
{
	return (playerActionStates[player][ActionName].state == Held);
}
bool System::IsIdle(std::string ActionName, int player)
{
	return (playerActionStates[player][ActionName].state == Idle);
}
bool System::IsReleased(std::string ActionName, int player)
{
	return (playerActionStates[player][ActionName].state == Relesased);
}
bool System::IsPressed(std::string ActionName, int player)
{
	return (playerActionStates[player][ActionName].state == Pressed);
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
	if (!device)
		return -1;
	DeviceType type = device->GetType();
	if (type == Keyboard)
		PlayerKeyboardPool[PlayerID] = { device };
	else if (type == Mouse)
		PlayerMousePool[PlayerID] = { device };
	else if (type == Gamepad)
		PlayerGamepadPool[PlayerID].push_back(device);
	return 0;
}
int System::AssignDevicesToPlayer(int PlayerID, std::vector<std::shared_ptr<Device>> devices)
{
	for (auto& x : devices)
		AssignDeviceToPlayer(PlayerID, x);
	return 0;
}
int System::RemoveDeviceFromPlayer(int playerID, std::shared_ptr<Device> device)
{
	if (!device)
		return -1;
	PlayerKeyboardPool[playerID].clear();
	PlayerMousePool[playerID].clear();
	PlayerGamepadPool[playerID].erase(
		std::remove(PlayerGamepadPool[playerID].begin(), PlayerGamepadPool[playerID].end(), device),
		PlayerGamepadPool[playerID].end());
	return 0;
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
	while (SDL_PollEvent(&ev)) {
		switch (ev.type) {
		case SDL_EVENT_KEY_DOWN:
		case SDL_EVENT_KEY_UP:
			if (KeyboardHub::Current())
				static_cast<KeyboardDevice*>(KeyboardHub::Current().get())
				->SetState((int)ev.key.scancode, ev.type == SDL_EVENT_KEY_DOWN);
			break;
		case SDL_EVENT_MOUSE_BUTTON_DOWN:
		case SDL_EVENT_MOUSE_BUTTON_UP:
			if (MouseHub::Current())
				static_cast<MouseDevice*>(MouseHub::Current().get())
				->SetButton((int)ev.button.button, ev.type == SDL_EVENT_MOUSE_BUTTON_DOWN);
			break;
		case SDL_EVENT_MOUSE_MOTION:
			if (MouseHub::Current()) {
				auto m = static_cast<MouseDevice*>(MouseHub::Current().get());
				m->SetAxis(0, ev.motion.x);
				m->SetAxis(1, ev.motion.y);
			}
			break;
		case SDL_EVENT_GAMEPAD_ADDED:
			GamepadHub::All().push_back(std::make_shared<GamepadDevice>(ev.gdevice.which));
			break;
		case SDL_EVENT_GAMEPAD_REMOVED:
			std::erase_if(GamepadHub::All(), [&](auto& d) { return d->GetId() == ev.gdevice.which; });
			break;
		case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
		case SDL_EVENT_GAMEPAD_BUTTON_UP:
			for (auto& dev : GamepadHub::All())
				if (dev->GetId() == ev.gbutton.which)
					static_cast<GamepadDevice*>(dev.get())
					->SetButton((int)ev.gbutton.button, ev.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
			break;
		case SDL_EVENT_GAMEPAD_AXIS_MOTION:
			for (auto& dev : GamepadHub::All())
				if (dev->GetId() == ev.gaxis.which)
					static_cast<GamepadDevice*>(dev.get())
					->SetAxis((int)ev.gaxis.axis, ev.gaxis.value / 32767.0f);
			break;
		}
	}

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
Action& InputSystem::Action::AddBinding(BindingType bindingType, DeviceType device, int key)
{
	bindings.push_back(Bindings(bindingType, device, 0, 1.0f, key));
	return *this;
}
Action& InputSystem::Action::AddBinding(BindingType bindingType, DeviceType device, int key, int componentIndex)
{
	bindings.push_back(Bindings(bindingType, device, componentIndex, 1.0f, key));
	return *this;
}
Action& InputSystem::Action::AddBinding(BindingType bindingType, DeviceType device, int key, int componentIndex, float scale)
{
	bindings.push_back(Bindings(bindingType, device, componentIndex, scale, key));
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

ActionStateClass Action::GetActionState(float dt, int player, ActionStateClass previousState,
	std::vector<std::shared_ptr<Device>> PlayerKeyboardPool,
	std::vector<std::shared_ptr<Device>> PlayerMousePool,
	std::vector<std::shared_ptr<Device>> PlayerGamepadPool)
{
	if (!active)
		return ActionStateClass{ ActionState::Idle, INPUT_DATA_4{{0, 0, 0, 0}} };

	INPUT_DATA_4 rawData = { { 0,0,0,0 } };
	bool isAnyBindingPressed = false;

	for (const auto& binding : bindings)
	{
		const auto* targetPool = &PlayerKeyboardPool;
		if (binding.device == Mouse)
			targetPool = &PlayerMousePool;
		else if (binding.device == Gamepad)
			targetPool = &PlayerGamepadPool;

		for (auto& device : *targetPool) {
			float val = binding.bindingType == Button
				? device->IsPressed(binding.key)
				: device->GetAxis(binding.key);

			if (val != 0.0f) {
				rawData[binding.componentIndex] += val * binding.scale;
				isAnyBindingPressed = true;
			}
		}
	}

	INPUT_DATA_4 processedData = rawData;
	for (auto& processor : processors)
		processor->Process(processedData);

	ActionState currentRawState = ActionState::Idle;
	if (isAnyBindingPressed)
		currentRawState = (previousState.state == Held || previousState.state == Pressed) ? Held : Pressed;
	else
		currentRawState = (previousState.state == Idle || previousState.state == Relesased) ? Idle : Relesased;

	for (auto& interaction : interactions)
	{
		interaction->Update(processedData, dt, currentRawState);
		interaction->TriggerEvents(processedData);
	}

	return ActionStateClass{ currentRawState, processedData };
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

std::vector<std::shared_ptr<Device>>& GamepadHub::All()
{
	return gamepads;
}
std::shared_ptr<Device>& GamepadHub::Current()
{
	return LastUsedGamepad;
}

std::shared_ptr<Device>& KeyboardHub::Current()
{
	return keyboard;
}
std::shared_ptr<Device>& MouseHub::Current()
{
	return mouse;
}

InputSystem::Internals::KeyboardDevice::KeyboardDevice(int _id) : Device(_id, DeviceType::Keyboard)
{
	state.resize(SDL_SCANCODE_COUNT, false);
}
void InputSystem::Internals::KeyboardDevice::SetState(int key, bool pressed)
{
	if (key < state.size())
		state[key] = pressed;
}
float InputSystem::Internals::KeyboardDevice::IsPressed(int key)
{
	return (key >= 0 && key < state.size() && state[key]) ? 1.0f : 0.0f;
}
float InputSystem::Internals::KeyboardDevice::GetAxis(int axis)
{
	return 0.0f;
}

InputSystem::Internals::MouseDevice::MouseDevice(int _id)
	: Device(_id, DeviceType::Mouse)
{
	buttons.resize(8, false);
}
void InputSystem::Internals::MouseDevice::SetButton(int btn, bool pressed)
{
	if (btn < buttons.size())
		buttons[btn] = pressed;
}
void InputSystem::Internals::MouseDevice::SetAxis(int idx, float val)
{
	if (idx < 4)
		axes[idx] = val;
}
float InputSystem::Internals::MouseDevice::IsPressed(int key)
{
	return (key >= 0 && key < buttons.size() && buttons[key]) ? 1.0f : 0.0f;
}
float InputSystem::Internals::MouseDevice::GetAxis(int axis)

{
	return (axis >= 0 && axis < 4) ? axes[axis] : 0.0f;
}

InputSystem::Internals::GamepadDevice::GamepadDevice(int _id)
	: Device(_id, DeviceType::Gamepad)
{
	buttons.resize(SDL_GAMEPAD_BUTTON_COUNT, false);
	axes.resize(SDL_GAMEPAD_AXIS_COUNT, 0.0f);
}
void InputSystem::Internals::GamepadDevice::SetButton(int btn, bool pressed)
{
	if (btn < buttons.size())
		buttons[btn] = pressed;
}
void InputSystem::Internals::GamepadDevice::SetAxis(int idx, float val)
{
	if (idx < axes.size())
		axes[idx] = val;
}
float InputSystem::Internals::GamepadDevice::IsPressed(int key)
{
	return (key >= 0 && key < buttons.size() && buttons[key]) ? 1.0f : 0.0f;
}
float InputSystem::Internals::GamepadDevice::GetAxis(int axis)
{
	return (axis >= 0 && axis < axes.size()) ? axes[axis] : 0.0f;
}