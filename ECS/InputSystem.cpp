#include "InputSystem.h"

using namespace InputSystem;
using namespace Internals;

std::shared_ptr<Device> KeyboardHub::keyboard = nullptr;
std::shared_ptr<Device> MouseHub::mouse = nullptr;

std::shared_ptr<Device> GamepadHub::LastUsedGamepad = nullptr;
std::vector<std::shared_ptr<Device>> GamepadHub::gamepads;

void System::Init()
{
	Update(0.0f);

	if (!KeyboardHub::Current()) {
		KeyboardHub::Current() = std::make_shared<KeyboardDevice>(0);
	}
	if (!MouseHub::Current()) {
		MouseHub::Current() = std::make_shared<MouseDevice>(0);
	}

	AssignDeviceToPlayer(KeyboardHub::Current());
	AssignDeviceToPlayer(MouseHub::Current());

	AddActionMap("default");
	AssignMapToPlayer("default");
	Update(0.0f);
}

ActionMap* System::GetActionMap(StringId ActionMapName)
{
	auto it = ActionMaps.find(ActionMapName);
	return it != ActionMaps.end() ? &it->second : nullptr;
}
int System::RemoveActionMap(StringId ActionMapName)
{
	auto it = ActionMaps.find(ActionMapName);
	if (it == ActionMaps.end())
		return -1;
	ActionMaps.erase(it);
	return 0;
}
ActionState System::GetActionState(StringId ActionName, int player)
{
	auto playerIt = playerActionStates.find(player);
	if (playerIt == playerActionStates.end())
		return ActionState::Idle;

	auto actionIt = playerIt->second.find(ActionName);
	if (actionIt == playerIt->second.end())
		return ActionState::Idle;

	return actionIt->second.state;
}
INPUT_DATA_4 System::GetActionAxis(StringId ActionName, int player)
{
	auto playerIt = playerActionStates.find(player);
	if (playerIt == playerActionStates.end())
		return INPUT_DATA_4{ { 0, 0, 0, 0 } };

	auto actionIt = playerIt->second.find(ActionName);
	if (actionIt == playerIt->second.end())
		return INPUT_DATA_4{ { 0, 0, 0, 0 } };

	return actionIt->second.data;
}

bool System::IsHeld(StringId ActionName, int player)
{
	auto playerIt = playerActionStates.find(player);
	if (playerIt == playerActionStates.end())
		return false;
	auto actionIt = playerIt->second.find(ActionName);
	if (actionIt == playerIt->second.end())
		return false;
	return actionIt->second.state == Held;
}
bool System::IsIdle(StringId ActionName, int player)
{
	auto playerIt = playerActionStates.find(player);
	if (playerIt == playerActionStates.end())
		return true;
	auto actionIt = playerIt->second.find(ActionName);
	if (actionIt == playerIt->second.end())
		return true;
	return actionIt->second.state == Idle;
}
bool System::IsPressed(StringId ActionName, int player)
{
	auto playerIt = playerActionStates.find(player);
	if (playerIt == playerActionStates.end())
		return false;
	auto actionIt = playerIt->second.find(ActionName);
	if (actionIt == playerIt->second.end())
		return false;
	return actionIt->second.state == Pressed;
}
bool System::IsReleased(StringId ActionName, int player)
{
	auto playerIt = playerActionStates.find(player);
	if (playerIt == playerActionStates.end())
		return false;
	auto actionIt = playerIt->second.find(ActionName);
	if (actionIt == playerIt->second.end())
		return false;
	return actionIt->second.state == Released;
}

ActionMap& System::AddActionMap(StringId ActionMapName)
{
	return ActionMaps[ActionMapName];
}

int System::AssignMapToPlayer(StringId Map, int PlayerID)
{
	PlayerToMap[PlayerID] = Map;
	return 0;
}
int System::AssignDeviceToPlayer(const std::shared_ptr<Device>& device, int PlayerID)
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
int System::AssignDevicesToPlayer(const std::vector<std::shared_ptr<Device>>& devices, int PlayerID)
{
	for (auto& x : devices)
		AssignDeviceToPlayer(x, PlayerID);
	return 0;
}


int System::RemoveDeviceFromPlayer(const std::shared_ptr<Device>& device, int playerID) {
	if (!device)
		return -1;

	auto removeFrom = [&](auto& pool) {
		auto it = pool.find(playerID);
		if (it == pool.end()) return;
		auto& vec = it->second;
		vec.erase(std::remove(vec.begin(), vec.end(), device), vec.end());
		};

	removeFrom(PlayerKeyboardPool);
	removeFrom(PlayerMousePool);
	removeFrom(PlayerGamepadPool);
	return 0;
}

std::vector<std::shared_ptr<Device>> System::GetDevicesOfType(int player, DeviceType type)
{
	const auto* pool = (type == Keyboard) ? &PlayerKeyboardPool
		: (type == Mouse) ? &PlayerMousePool
		: &PlayerGamepadPool;
	auto it = pool->find(player);
	if (it == pool->end())
		return {};
	return it->second;
}

void System::Update(float dt)
{
	if (MouseHub::Current())
		static_cast<MouseDevice*>(MouseHub::Current().get())->ResetFrameAxes();

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
				m->SetAxis(4, m->GetAxis(4) + ev.motion.xrel);
				m->SetAxis(5, m->GetAxis(5) + ev.motion.yrel);
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
				if (dev->GetId() == ev.gbutton.which) {
					static_cast<GamepadDevice*>(dev.get())
						->SetButton((int)ev.gbutton.button, ev.type == SDL_EVENT_GAMEPAD_BUTTON_DOWN);
					GamepadHub::Current() = dev;
				}
			break;
		case SDL_EVENT_GAMEPAD_AXIS_MOTION:
			for (auto& dev : GamepadHub::All())
				if (dev->GetId() == ev.gaxis.which) {
					static_cast<GamepadDevice*>(dev.get())
						->SetAxis((int)ev.gaxis.axis, ev.gaxis.value / 32767.0f);
					GamepadHub::Current() = dev;
				}
			break;
		case SDL_EVENT_MOUSE_WHEEL:
			if (MouseHub::Current()) {
				auto m = static_cast<MouseDevice*>(MouseHub::Current().get());
				m->SetAxis(2, m->GetAxis(2) + ev.wheel.x);
				m->SetAxis(3, m->GetAxis(3) + ev.wheel.y);
			}
			break;
		}
	}

	for (auto& [playerId, mapName] : PlayerToMap)
	{
		auto mapIt = ActionMaps.find(mapName);
		if (mapIt == ActionMaps.end())
			continue;

		static const std::vector<std::shared_ptr<Device>> emptyPool{};
		auto kbIt = PlayerKeyboardPool.find(playerId);
		auto mIt = PlayerMousePool.find(playerId);
		auto gpIt = PlayerGamepadPool.find(playerId);

		mapIt->second.UpdateAllActionStates(
			dt,
			playerActionStates[playerId],
			kbIt != PlayerKeyboardPool.end() ? kbIt->second : emptyPool,
			mIt != PlayerMousePool.end() ? mIt->second : emptyPool,
			gpIt != PlayerGamepadPool.end() ? gpIt->second : emptyPool
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

void ActionMap::UpdateAllActionStates(float dt,
	std::unordered_map<StringId, ActionStateClass>& states,
	const std::vector<std::shared_ptr<Device>>& PlayerKeyboardPool,
	const std::vector<std::shared_ptr<Device>>& PlayerMousePool,
	const std::vector<std::shared_ptr<Device>>& PlayerGamepadPool)
{
	if (!active) {
		states.clear();
		return;
	}

	static const ActionStateClass defaultState{};
	for (auto& [name, action] : Actions)
	{
		auto it = states.find(name);
		const ActionStateClass& prev = (it != states.end()) ? it->second : defaultState;
		states[name] = action.GetActionState(dt, prev, PlayerKeyboardPool, PlayerMousePool, PlayerGamepadPool);
	}
}
Action& ActionMap::AddAction(StringId name)
{
	return Actions[name];
}
Action* ActionMap::GetAction(StringId name)
{
	auto it = Actions.find(name);
	return it != Actions.end() ? &it->second : nullptr;
}
int ActionMap::RemoveAction(StringId name)
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
int Action::RemoveProcessor(StringId name)
{
	auto it = std::find_if(processors.begin(), processors.end(),
		[&](const auto& p) { return p->name == name; });
	if (it == processors.end()) return -1;
	processors.erase(it);
	return 0;
}
int Action::RemoveInteraction(StringId name)
{
	auto it = std::find_if(interactions.begin(), interactions.end(),
		[&](const auto& i) { return i->name == name; });
	if (it == interactions.end()) return -1;
	interactions.erase(it);
	return 0;
}

bool Action::IsActive()
{
	return active;
}

Internals::ActionStateClass InputSystem::Action::GetActionState(float dt,
	const Internals::ActionStateClass& previousState,
	const std::vector<std::shared_ptr<Internals::Device>>& PlayerKeyboardPool,
	const std::vector<std::shared_ptr<Internals::Device>>& PlayerMousePool,
	const std::vector<std::shared_ptr<Internals::Device>>& PlayerGamepadPool)

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

			if (val != 0.0f && binding.componentIndex < 4 && binding.componentIndex >= 0) {
				rawData[binding.componentIndex] += val * binding.scale;
				if (binding.bindingType == Button)
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
		currentRawState = (previousState.state == Idle || previousState.state == Released) ? Idle : Released;

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

DeviceType Device::GetType() const
{
	return type;
}
int Device::GetId() const
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
	if (idx >= 0 && idx < 6)
		axes[idx] = val;
}
float InputSystem::Internals::MouseDevice::IsPressed(int key)
{
	return (key >= 0 && key < buttons.size() && buttons[key]) ? 1.0f : 0.0f;
}
float InputSystem::Internals::MouseDevice::GetAxis(int axis)
{
	return (axis >= 0 && axis < 6) ? axes[axis] : 0.0f;
}

void InputSystem::Internals::MouseDevice::ResetFrameAxes()
{
	axes[2] = 0.0f;  // scroll X
	axes[3] = 0.0f;  // scroll Y
	axes[4] = 0.0f;  // delta X
	axes[5] = 0.0f;  // delta Y
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