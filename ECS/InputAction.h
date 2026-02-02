#pragma once
#include <vector>
#include <string>
#include <SDL3/SDL_scancode.h>
#include <memory>
#include <unordered_map>

enum class DeviceType : uint8_t { Keyboard, Mouse, Gamepad };

struct InputID {
	DeviceType device;
	int code;

	bool operator==(const InputID& other) const {
		return device == other.device && code == other.code;
	}
};

struct ActionValue {
	float axes[4] = { 0, 0, 0, 0 };
	uint8_t size = 1;
};

class InputProcessor
{
public:
	virtual ~InputProcessor() = default;
	virtual void Process(ActionValue& value) const = 0;
};

class InputAction
{
private:
	std::vector<std::unique_ptr<InputProcessor>> Processors;
	std::vector<std::string> Interactions;
	std::vector<InputID> Bindings;
public:

	InputAction() = default;
	bool Enabled;
	InputAction& AddBinding(DeviceType type, int code);
	InputAction& AddProcessor(std::unique_ptr<InputProcessor> p);
	//InputAction& AddInteraction(std::unique_ptr<InputProcessor> p);
	ActionValue Update();
	void Enable();
	void Disable();
};



class ActionMap
{
public:
	ActionMap() = default;
	bool Enabled;
	std::unordered_map<std::string, InputAction> actions;
	InputAction& CreateAction(std::string ActionName);
	void Enable();
	void Disable();
};