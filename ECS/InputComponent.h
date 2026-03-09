#pragma once
#include <cstdint>

struct InputComponent
{
    uint32_t PlayerID;
    InputComponent(uint32_t _player = -1): PlayerID(_player) {};
};