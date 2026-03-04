#pragma once
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include <array> 
#include <optional>
#include <functional>
#include "SDL3/SDL.h"

template <typename T>
using OptionalRef = std::optional<std::reference_wrapper<T>>;

enum DeviceType
{
	Keyboard,
	Mouse,
	Gamepad
};

enum InteractionResult
{
	None,
	Started,
	Performed,
	Canceled
};

enum ActionState
{
	Idle,
	Pressed,
	Relesased,
	Held,
	Error
};

enum BindingType
{
	Button,
	Axis
};


struct INPUT_DATA_4 { std::array<float, 4> data; };

struct Bindings {
	BindingType bindingType;
	DeviceType device;
	int componentIndex;
	float scale = 1.0f;
	int key;
	bool operator==(const Bindings& other) const
	{
		return ((device == other.device) && (key == other.key));
	}
};



class Processor {
public:
	std::string name;

};

class Interaction {
public:
	InteractionResult result = None;
	std::string name;
	std::function<void(INPUT_DATA_4)> OnStarted;
	std::function<void(INPUT_DATA_4)> OnPerformed;
	std::function<void(INPUT_DATA_4)> OnCanceled;

	virtual void Update(const INPUT_DATA_4& data, float dt, ActionState rawState) = 0;

	void TriggerEvents(const INPUT_DATA_4& data) {
		if (result == InteractionResult::Started && OnStarted)
			OnStarted(data);
		if (result == InteractionResult::Performed && OnPerformed)
			OnPerformed(data);
		if (result == InteractionResult::Canceled && OnCanceled)
			OnCanceled(data);
	}

	virtual ~Interaction() = default;

	bool operator==(const Interaction& other) const
	{
		return (name == other.name);
	}
};

class Device
{
	DeviceType type;
	int DeviceId;
public:
	Device(int _DeviceId, DeviceType _type) : type(_type), DeviceId(_DeviceId) {};
	DeviceType GetType();
	int GetId();
	virtual float IsPressed(int key) = 0;
	virtual float GetAxis(int axis) = 0;
};


class KeyboardDevice : public Device
{
	std::vector<bool> state;
public:
	KeyboardDevice(int _id) : Device(_id, DeviceType::Keyboard) {
		state.resize(SDL_SCANCODE_COUNT, false);
	}

	void SetState(int key, bool pressed)
	{
		if (key < state.size())
			state[key] = pressed;
	}

	float IsPressed(int key) override
	{
		return (key >= 0 && key < state.size() && state[key]) ? 1.0f : 0.0f;
	}
	float GetAxis(int axis) override { return 0.0f; }
};

class MouseDevice : public Device {
	std::vector<bool> buttons;
	float axes[4] = { 0 }; // 0:X, 1:Y, 2:WheelX, 3:WheelY
public:
	MouseDevice(int _id) : Device(_id, DeviceType::Mouse)
	{
		buttons.resize(8, false);
	}

	void SetButton(int btn, bool pressed)
	{
		if (btn < buttons.size())
			buttons[btn] = pressed;
	}
	void SetAxis(int idx, float val)
	{
		if (idx < 4)
			axes[idx] = val;
	}

	float IsPressed(int key) override
	{
		return (key >= 0 && key < buttons.size() && buttons[key]) ? 1.0f : 0.0f;
	}
	float GetAxis(int axis) override
	{
		return (axis >= 0 && axis < 4) ? axes[axis] : 0.0f;
	}
};

class GamepadDevice : public Device {
	std::vector<bool> buttons;
	std::vector<float> axes;
public:
	GamepadDevice(int _id) : Device(_id, DeviceType::Gamepad)
	{
		buttons.resize(SDL_GAMEPAD_BUTTON_COUNT, false);
		axes.resize(SDL_GAMEPAD_AXIS_COUNT, 0.0f);
	}

	void SetButton(int btn, bool pressed)
	{
		if (btn < buttons.size())
			buttons[btn] = pressed;
	}
	void SetAxis(int idx, float val)
	{
		if (idx < axes.size())
			axes[idx] = val;
	}

	float IsPressed(int key) override
	{
		return (key >= 0 && key < buttons.size() && buttons[key]) ? 1.0f : 0.0f;
	}
	float GetAxis(int axis) override
	{
		return (axis >= 0 && axis < axes.size()) ? axes[axis] : 0.0f;
	}
};

static class Keyboards
{
	static std::shared_ptr<Device> LastUsedKeyboard;
	static std::vector<std::shared_ptr<Device>> keyboards;

public:
	static std::vector<std::shared_ptr<Device>>& All();
	static std::shared_ptr<Device> Current();
};

static class Mice
{
	static std::shared_ptr<Device> LastUsedMouse;
	static std::vector<std::shared_ptr<Device>> mice;
public:
	static std::vector<std::shared_ptr<Device>>& All();
	static std::shared_ptr<Device> Current();
};

static class Gamepads
{
	static std::shared_ptr<Device> LastUsedGamepad;
	static std::vector<std::shared_ptr<Device>> gamepads;
public:
	static std::vector<std::shared_ptr<Device>>& All();
	static std::shared_ptr<Device> Current();
};


class Action
{
	friend class InputSystem;
	bool active;
	std::vector<Bindings> bindings;
	std::vector<std::unique_ptr<Processor>> processors;
	std::vector<std::unique_ptr<Interaction>> interactions;
	ActionState state;
	INPUT_DATA_4 data;

public:
	Action() = default;
	Action& AddBinding(Bindings binding);
	Action& AddProcessor(std::unique_ptr<Processor> processor);
	Action& AddInteraction(std::unique_ptr<Interaction> interaction);

	int RemoveBinding(Bindings binding);
	int RemoveProcessor(std::string name);
	int RemoveInteraction(std::string name);

	void Update(int dt);

	void Activate();
	void Deactivate();

	ActionState GetState();
	INPUT_DATA_4 GetActionAxis();

	bool IsActive();
};

class ActionMap
{
	bool active;
	std::unordered_map<std::string, Action> Actions;

public:
	ActionMap() = default;
	Action& AddAction(std::string name);
	OptionalRef<Action> GetAction(std::string name);
	int RemoveAction(std::string name);

	ActionState GetActionState(std::string ActionName);
	INPUT_DATA_4 GetActionAxis(std::string ActionName);

	void Update(int dt);

	void Activate();
	void Deactivate();

	bool IsActive();
};



class InputSystem
{
public:
	InputSystem() = default;
	ActionMap defaultActionMap;
	std::unordered_map<std::string, ActionMap> ActionMaps;
	std::unordered_map<int, std::vector<std::shared_ptr<Device>>> PlayerKeyboardPool;
	std::unordered_map<int, std::vector<std::shared_ptr<Device>>> PlayerMousePool;
	std::unordered_map<int, std::vector<std::shared_ptr<Device>>> PlayerGamepadPool;

	std::unordered_map<int, std::string> PlayerToMap;

	ActionMap& GetActionMap(std::string ActionMapName);
	OptionalRef<ActionMap> AddActionMap(std::string ActionMapName);
	int RemoveActionMap(std::string ActionMapName);

	ActionState GetActionState(int player, std::string ActionName);
	ActionState GetActionState(std::string ActionName);

	INPUT_DATA_4 GetActionAxis(int player, std::string ActionName);
	INPUT_DATA_4 GetActionAxis(std::string ActionName);


	int AssignMapToPlayer(int PlayerID, std::string Map);
	int AssignDeviceToPlayer(int PlayerID, std::shared_ptr<Device> device);
	int RemoveDeviceFromPlayer(int playerID, std::shared_ptr<Device> device);

	std::vector<std::shared_ptr<Device>> GetDevicesOfType(int player, DeviceType type);

	void Update(int dt);
};

