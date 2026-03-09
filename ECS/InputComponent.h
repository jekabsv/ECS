#pragma once
#include "InputSystem.h"
#include "Struct.h"

struct InputComponent
{
    uint32_t PlayerID;
    InputSystem::System* system;
    InputComponent(uint32_t _player = -1): PlayerID(_player) {};
};