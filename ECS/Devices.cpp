#include "Devices.h"
#include <cassert>

std::vector<InputDevice*> Keyboard::Devices;
std::vector<InputDevice*> Mouse::Devices;
std::vector<InputDevice*> Gamepad::Devices;

InputDevice* Keyboard::CurrentDevice = nullptr;
InputDevice* Mouse::CurrentDevice = nullptr;
InputDevice* Gamepad::CurrentDevice = nullptr;

InputDevice& Keyboard::Current()
{
    assert(CurrentDevice);
    return *CurrentDevice;
}
const std::vector<InputDevice*>& Keyboard::All() { return Devices; }

InputDevice& Mouse::Current()
{
    assert(CurrentDevice);
    return *CurrentDevice;
}
const std::vector<InputDevice*>& Mouse::All() { return Devices; }

InputDevice& Gamepad::Current()
{
    assert(CurrentDevice);
    return *CurrentDevice;
}
const std::vector<InputDevice*>& Gamepad::All() { return Devices; }
