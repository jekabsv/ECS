#pragma once
#include <unordered_map>
#include <vector>
#include <string>
#include "InputAction.h"
#include "InputDevice.h"

using UserID = uint32_t;

struct UserContext
{
    std::string ActionMap;
    std::vector<InputDevice*> Devices;
};

class InputSystem
{
public:
    void Init();

    ActionMap& CreateActionMap(const std::string& name);
    void DeleteActionMap(const std::string& name);

    void SubscribeUser(UserID user, const std::string& map);

    void AssignDevice(UserID user, InputDevice& device);
    void UnassignDevice(InputDevice& device);

    void OnGamepadConnected(int index);
    void OnGamepadDisconnected(int instanceID);

    void Update();
    ActionValue GetAction(UserID user, const std::string& action) const;

private:
    std::unordered_map<std::string, ActionMap> ActionMaps;
    std::unordered_map<UserID, UserContext> Users;
};
