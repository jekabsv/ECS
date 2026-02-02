#include "InputSystem.h"

ActionMap& InputSystem::CreateActionMap(std::string name)
{
    ActionMaps[name] = ActionMap();
    return ActionMaps[name];
}
