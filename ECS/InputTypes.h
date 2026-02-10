#pragma once
#include <cstdint>
#include <SDL3/SDL.h>
enum class DeviceType : uint8_t
{
    Keyboard,
    Mouse,
    Gamepad,
    Joystick
};

enum class ButtonState : uint8_t
{
    Idle,
    Pressed,
    Held,
    Released
};

struct InputID
{
    DeviceType device;
    int code;
};

struct ActionValue
{
    float axes[4]{};
    uint8_t size = 0;
};

class InputProcessor
{
public:
    virtual ~InputProcessor() = default;
    virtual void Process(ActionValue& value) const = 0;
};
