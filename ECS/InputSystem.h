#pragma once
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include <array> 
#include <optional>
#include <functional>
#include <algorithm>
#include "SDL3/SDL.h"

namespace InputSystem
{

	template <typename T>
	using OptionalRef = std::optional<std::reference_wrapper<T>>;

	enum DeviceType { Keyboard, Mouse, Gamepad };
	enum InteractionResult { None, Started, Performed, Canceled };
	enum ActionState { Idle, Pressed, Relesased, Held, Error };
	enum BindingType { Button, Axis };

	using INPUT_DATA_4 = std::array<float, 4>;

	struct Bindings
	{
		BindingType bindingType;
		DeviceType device;
		int componentIndex;
		float scale = 1.0f;
		int key;
		Bindings(BindingType _bindingType, DeviceType _device, int _componentIndex, float _scale, int _key) :
			bindingType(_bindingType), device(_device), componentIndex(_componentIndex), scale(_scale), key(_key) {
		};
		Bindings(BindingType _bindingType, DeviceType _device, int _componentIndex, int _key) :
			bindingType(_bindingType), device(_device), componentIndex(_componentIndex), scale(1.0f), key(_key) {
		};
		bool operator==(const Bindings& other) const
		{
			return ((device == other.device) && (key == other.key));
		}
	};

	namespace Internals {
		class ActionStateClass
		{
		public:
			ActionStateClass(ActionState _state, INPUT_DATA_4 _data) : data(_data), state(_state) {};
			ActionStateClass()
				: state(ActionState::Idle), data{ { 0,0,0,0 } } {
			}
			ActionState state;
			INPUT_DATA_4 data;
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
	}


	class Processor {
	public:
		std::string name;
		virtual int Process(INPUT_DATA_4& data)
		{
			return 0;
		}
	};

	class Interaction {
	public:
		Interaction(std::string _name) : name(_name) {};
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


	class Keyboards
	{
		static std::shared_ptr<Internals::Device> LastUsedKeyboard;
		static std::vector<std::shared_ptr<Internals::Device>> keyboards;

	public:
		static std::vector<std::shared_ptr<Internals::Device>>& All();
		static std::shared_ptr<Internals::Device>& Current();
	};
	class Mice
	{
		static std::shared_ptr<Internals::Device> LastUsedMouse;
		static std::vector<std::shared_ptr<Internals::Device>> mice;
	public:
		static std::vector<std::shared_ptr<Internals::Device>>& All();
		static std::shared_ptr<Internals::Device>& Current();
	};
	class Gamepads
	{
		static std::shared_ptr<Internals::Device> LastUsedGamepad;
		static std::vector<std::shared_ptr<Internals::Device>> gamepads;
	public:
		static std::vector<std::shared_ptr<Internals::Device>>& All();
		static std::shared_ptr<Internals::Device>& Current();
	};


	class Action
	{
		bool active = true;
		std::vector<Bindings> bindings;
		std::vector<std::unique_ptr<Processor>> processors;
		std::vector<std::unique_ptr<Interaction>> interactions;
	public:
		Action() = default;
		Action& AddBinding(Bindings binding);
		Action& AddProcessor(std::unique_ptr<Processor> processor);
		Action& AddInteraction(std::unique_ptr<Interaction> interaction);

		int RemoveBinding(Bindings binding);
		int RemoveProcessor(std::string name);
		int RemoveInteraction(std::string name);

		void Activate();
		void Deactivate();
		bool IsActive();

		Internals::ActionStateClass GetActionState(float dt, int player, Internals::ActionStateClass previousState, std::vector<std::shared_ptr<Internals::Device>> PlayerKeyboardPool,
			std::vector<std::shared_ptr<Internals::Device>> PlayerMousePool, std::vector<std::shared_ptr<Internals::Device>> PlayerGamepadPool);
	};
	class ActionMap
	{
		bool active = true;
		std::unordered_map<std::string, Action> Actions;

	public:
		ActionMap() = default;
		Action& AddAction(std::string name);
		OptionalRef<Action> GetAction(std::string name);
		int RemoveAction(std::string name);

		void Activate();
		void Deactivate();
		bool IsActive();

		std::unordered_map<std::string, Internals::ActionStateClass> GetAllActionStates(float dt, int player, std::unordered_map<std::string, Internals::ActionStateClass>,
			std::vector<std::shared_ptr<Internals::Device>>& PlayerKeyboardPool,
			std::vector<std::shared_ptr<Internals::Device>>& PlayerMousePool, std::vector<std::shared_ptr<Internals::Device>>& PlayerGamepadPool);
	};

	class System
	{
	public:
		System() = default;

		std::unordered_map<std::string, ActionMap> ActionMaps;
		std::unordered_map<int, std::vector<std::shared_ptr<Internals::Device>>> PlayerKeyboardPool;
		std::unordered_map<int, std::vector<std::shared_ptr<Internals::Device>>> PlayerMousePool;
		std::unordered_map<int, std::vector<std::shared_ptr<Internals::Device>>> PlayerGamepadPool;

		void Init(float dt);

		//default is on playerId -1
		std::unordered_map<int, std::string> PlayerToMap;
		std::unordered_map<int, std::unordered_map<std::string, Internals::ActionStateClass>> playerActionStates;

		ActionMap& GetActionMap(std::string ActionMapName);
		ActionMap& AddActionMap(std::string ActionMapName);
		int RemoveActionMap(std::string ActionMapName);

		ActionState GetActionState(int player, std::string ActionName);
		ActionState GetActionState(std::string ActionName); // return for default (-1)

		INPUT_DATA_4 GetActionAxis(int player, std::string ActionName);
		INPUT_DATA_4 GetActionAxis(std::string ActionName); // return for default (-1)


		int AssignMapToPlayer(int PlayerID, std::string Map);
		int AssignDeviceToPlayer(int PlayerID, std::shared_ptr<Internals::Device> device);
		int AssignDevicesToPlayer(int PlayerID, std::vector<std::shared_ptr<Internals::Device>>);
		int RemoveDeviceFromPlayer(int playerID, std::shared_ptr<Internals::Device> device);

		std::vector<std::shared_ptr<Internals::Device>> GetDevicesOfType(int player, DeviceType type);

		void Update(float dt);
	};

}
