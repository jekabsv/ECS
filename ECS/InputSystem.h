#pragma once

#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include <array> 
#include <functional>
#include "Struct.h"

union SDL_Event;

namespace InputSystem
{

	enum DeviceType { Keyboard, Mouse, Gamepad };
	enum InteractionResult { None, Started, Performed, Canceled };
	enum ActionState { Idle, Pressed, Released, Held, Error };
	enum BindingType { Button, Axis };

	using INPUT_DATA_4 = std::array<float, 4>;

	struct Bindings
	{
		BindingType bindingType;
		DeviceType device;
		int componentIndex;
		float scale = 1.0f;
		int key;
		Bindings(BindingType _bindingType, DeviceType _device, int _componentIndex, float _scale, int _key);
		Bindings(BindingType _bindingType, DeviceType _device, int _componentIndex, int _key);
		bool operator==(const Bindings& other) const;
	};

	namespace Internals {
		class ActionStateClass
		{
		public:
			ActionStateClass(ActionState _state, INPUT_DATA_4 _data);
			ActionStateClass();
			ActionState state;
			INPUT_DATA_4 data;
		};

		class Device
		{
			DeviceType type;
			int DeviceId;
		public:
			Device(int _DeviceId, DeviceType _type);
			DeviceType GetType() const;
			int GetId() const;
			virtual float IsPressed(int key) = 0;
			virtual float GetAxis(int axis) = 0;
		};

		class KeyboardDevice : public Device
		{
			std::vector<bool> state;
		public:
			KeyboardDevice(int _id);

			void SetState(int key, bool pressed);
			float IsPressed(int key) override;
			float GetAxis(int axis) override;
		};

		class MouseDevice : public Device {
			std::vector<bool> buttons;
			float axes[6] = { 0 };
		public:
			MouseDevice(int _id);
			void SetButton(int btn, bool pressed);
			void SetAxis(int idx, float val);
			float IsPressed(int key) override;
			float GetAxis(int axis) override;
			void ResetFrameAxes();
		};

		class GamepadDevice : public Device
		{
			std::vector<bool> buttons;
			std::vector<float> axes;
		public:
			GamepadDevice(int _id);
			void SetButton(int btn, bool pressed);
			void SetAxis(int idx, float val);
			float IsPressed(int key) override;
			float GetAxis(int axis) override;
		};
	}

	class Processor {
	public:
		Processor(StringId _name) : name(_name) {};
		StringId name;
		virtual int Process(INPUT_DATA_4& data);
	};

	class Interaction {
	public:
		Interaction(StringId _name) : name(_name) {};
		InteractionResult result = None;
		StringId name;
		std::function<void(INPUT_DATA_4)> OnStarted;
		std::function<void(INPUT_DATA_4)> OnPerformed;
		std::function<void(INPUT_DATA_4)> OnCanceled;

		virtual void Update(const INPUT_DATA_4& data, float dt, ActionState rawState) = 0;

		void TriggerEvents(const INPUT_DATA_4& data);

		virtual ~Interaction() = default;

		bool operator==(const Interaction& other) const;
	};

	class KeyboardHub
	{
		static std::shared_ptr<Internals::Device> keyboard;
	public:
		static std::shared_ptr<Internals::Device>& Current();
	};
	class MouseHub
	{
		static std::shared_ptr<Internals::Device> mouse;
	public:
		static std::shared_ptr<Internals::Device>& Current();
	};
	class GamepadHub
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
		Action& AddBinding(BindingType bindingType, DeviceType device, int key);
		Action& AddBinding(BindingType bindingType, DeviceType device, int key, int componentIndex);
		Action& AddBinding(BindingType bindingType, DeviceType device, int key, int componentIndex, float scale);
		Action& AddProcessor(std::unique_ptr<Processor> processor);
		Action& AddInteraction(std::unique_ptr<Interaction> interaction);

		int RemoveBinding(Bindings binding);
		int RemoveProcessor(StringId name);
		int RemoveInteraction(StringId name);

		void Activate();
		void Deactivate();
		bool IsActive();

		Internals::ActionStateClass GetActionState(float dt,
			const Internals::ActionStateClass& previousState,
			const std::vector<std::shared_ptr<Internals::Device>>& PlayerKeyboardPool,
			const std::vector<std::shared_ptr<Internals::Device>>& PlayerMousePool,
			const std::vector<std::shared_ptr<Internals::Device>>& PlayerGamepadPool);
	};
	class ActionMap
	{
		bool active = true;
		std::unordered_map<StringId, Action> Actions;

	public:
		ActionMap() = default;
		Action& AddAction(StringId name);
		Action* GetAction(StringId name);
		int RemoveAction(StringId name);

		void Activate();
		void Deactivate();
		bool IsActive();

		void UpdateAllActionStates(float dt,
			std::unordered_map<StringId, Internals::ActionStateClass>& states,
			const std::vector<std::shared_ptr<Internals::Device>>& PlayerKeyboardPool,
			const std::vector<std::shared_ptr<Internals::Device>>& PlayerMousePool,
			const std::vector<std::shared_ptr<Internals::Device>>& PlayerGamepadPool);
	};

	class System
	{
		std::unordered_map<StringId, ActionMap> ActionMaps;
		std::unordered_map<int, std::vector<std::shared_ptr<Internals::Device>>> PlayerKeyboardPool;
		std::unordered_map<int, std::vector<std::shared_ptr<Internals::Device>>> PlayerMousePool;
		std::unordered_map<int, std::vector<std::shared_ptr<Internals::Device>>> PlayerGamepadPool;

		std::unordered_map<int, StringId> PlayerToMap;
		std::unordered_map<int, std::unordered_map<StringId, Internals::ActionStateClass>> playerActionStates;

	public:
		System() = default;

		void Init();

		ActionMap* GetActionMap(StringId ActionMapName = "default");
		ActionMap& AddActionMap(StringId ActionMapName);
		int RemoveActionMap(StringId ActionMapName);

		ActionState GetActionState(StringId ActionName, int player = -1);
		INPUT_DATA_4 GetActionAxis(StringId ActionName, int player = -1);

		bool IsHeld(StringId ActionName, int player = -1);
		bool IsIdle(StringId ActionName, int player = -1);
		bool IsPressed(StringId ActionName, int player = -1);
		bool IsReleased(StringId ActionName, int player = -1);

		int AssignMapToPlayer(StringId Map, int PlayerID = -1);
		int AssignDeviceToPlayer(const std::shared_ptr<Internals::Device>& device, int PlayerID = -1);
		int AssignDevicesToPlayer(const std::vector<std::shared_ptr<Internals::Device>>&, int PlayerID = -1);
		int RemoveDeviceFromPlayer(const std::shared_ptr<Internals::Device>& device, int playerID = -1);

		std::vector<std::shared_ptr<Internals::Device>> GetDevicesOfType(int player, DeviceType type);

		
		void Update(float dt);
		void HandleEvent(SDL_Event& event, bool &quit);
	};

}