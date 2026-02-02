#include "InputAction.h"

InputAction& InputAction::AddBinding(DeviceType type, int code)
{
    Bindings.push_back({ type, code });
    return *this;
}

InputAction& InputAction::AddProcessor(std::unique_ptr<InputProcessor> p)
{
    Processors.push_back(std::move(p));
    return *this;
}

void InputAction::Enable()
{
    Enabled = true;
}
void InputAction::Disable()
{
    Enabled = false;
}

InputAction& ActionMap::CreateAction(std::string ActionName)
{
    actions[ActionName] = InputAction();
    return actions[ActionName];
}

void ActionMap::Enable()
{
    Enabled = true;
}
void ActionMap::Disable()
{
    Enabled = false;
}
