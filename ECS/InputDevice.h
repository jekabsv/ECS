#pragma once
#include "InputTypes.h"

class InputDevice
{
public:
    virtual ~InputDevice() = default;

    virtual DeviceType GetType() const = 0;

    virtual void Poll() = 0;
    virtual bool IsActiveThisFrame() const = 0;

    virtual bool IsDown(int code) const = 0;
    virtual float GetAxis(int axis) const { return 0.0f; }
};
