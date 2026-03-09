#pragma once
#include <vector>
#include <unordered_map>
#include <string>

struct InputComponent
{
    struct ActionState 
    {
        bool IsPressed;   
        bool IsHeld;
        bool IsReleased;
        bool IsIdle;
    };
    std::unordered_map<std::string, ActionState> actions;
};