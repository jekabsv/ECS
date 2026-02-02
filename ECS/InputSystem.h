#pragma once
#include "InputAction.h"

class InputSystem
{
	std::unordered_map<std::string, ActionMap> ActionMaps;
	ActionMap& CreateActionMap(std::string name);
};