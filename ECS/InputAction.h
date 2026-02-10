#pragma once
#include <unordered_map>
#include <vector>
#include <memory>
#include <string>
#include "InputTypes.h"

class InputSystem;

class InputAction
{
    friend class InputSystem;

    bool Enabled = true;
    std::vector<InputID> Bindings;
    std::vector<std::unique_ptr<InputProcessor>> Processors;

public:
    InputAction& AddBinding(DeviceType dev, int code);
    InputAction& AddProcessor(std::unique_ptr<InputProcessor> p);

    void Enable();
    void Disable();
};

class ActionMap
{
    friend class InputSystem;

    bool Enabled = true;
    std::unordered_map<std::string, InputAction> Actions;

public:
    InputAction& CreateAction(const std::string& name);
    InputAction& GetAction(const std::string& name);
    void DeleteAction(const std::string& name);

    void Enable();
    void Disable();
};
