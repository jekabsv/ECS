#include "InputAction.h"

InputAction& InputAction::AddBinding(DeviceType dev, int code)
{
    Bindings.push_back({ dev, code });
    return *this;
}

InputAction& InputAction::AddProcessor(std::unique_ptr<InputProcessor> p)
{
    Processors.push_back(std::move(p));
    return *this;
}

void InputAction::Enable() { Enabled = true; }
void InputAction::Disable() { Enabled = false; }

InputAction& ActionMap::CreateAction(const std::string& name)
{
    return Actions[name];
}

InputAction& ActionMap::GetAction(const std::string& name)
{
    return Actions.at(name);
}

void ActionMap::DeleteAction(const std::string& name)
{
    Actions.erase(name);
}

void ActionMap::Enable() { Enabled = true; }
void ActionMap::Disable() { Enabled = false; }
