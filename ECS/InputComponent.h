#pragma once
#include "Struct.h"
#include <unordered_map>
#include "InputSystem.h"

struct InputComponent
{
	uint32_t PlayerID;
	InputSystem::System& system;
	InputComponent(uint32_t player, InputSystem::System& data) : PlayerID(player), system(data) {};
	const InputSystem::INPUT_DATA_4 GetActionAxis(StringId ActionName)
	{
		auto it = system.playerActionStates.find(PlayerID);
		if (it == system.playerActionStates.end())
			return { 0, 0, 0, 0 };
		auto it2 = it->second.find(ActionName);
		if (it2 != it->second.end())
			return it2->second.data;
		return {0, 0, 0, 0};
	}
	const InputSystem::ActionState GetActionState(StringId ActionName)
	{
		auto it = system.playerActionStates.find(PlayerID);
		if (it == system.playerActionStates.end())
			return InputSystem::Idle;
		auto it2 = it->second.find(ActionName);
		if (it2 != it->second.end())
			return it2->second.state;
		return InputSystem::Idle;
	}
};