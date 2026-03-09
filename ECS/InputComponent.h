#pragma once
#include "InputSystem.h"
#include "Struct.h"

struct InputComponent
{
    uint32_t PlayerID;
    InputSystem::System* system;
    InputComponent(InputSystem::System* _system, uint32_t _player = -1) : system(_system), PlayerID(_player) {};
    InputSystem::INPUT_DATA_4 GetActionAxis(StringId ActionName)
    {
        return system->GetActionAxis(ActionName, PlayerID);
    }
    InputSystem::ActionState GetActionState(StringId ActionName)
    {
        return system->GetActionState(ActionName, PlayerID);
    }
};