#pragma once
#include <vector>
#include "InputDevice.h"


#include <SDL3/SDL.h>
#include <unordered_map>

class KeyboardDevice : public InputDevice 
{
    SDL_KeyboardID id;
    bool active = false;
    const bool* state = nullptr;
public:
    KeyboardDevice(SDL_KeyboardID instanceID) : id(instanceID) {}

    DeviceType GetType() const override {
        return DeviceType::Keyboard;
    }

    SDL_KeyboardID GetID() const {
        return id;
    }

    void Poll() override { state = SDL_GetKeyboardState(nullptr); }

    bool IsActiveThisFrame() const override {
        return active;
    }

    bool IsDown(int scancode) const override {
        return state[scancode];
    }
};

class MouseDevice : public InputDevice {
    SDL_MouseID id;
    bool active = false;
    Uint32 state = 0;
    float x = 0, y = 0;

public:

    MouseDevice(SDL_MouseID instanceID) : id(instanceID) {}

    DeviceType GetType() const override {
        return DeviceType::Mouse;
    }

    SDL_MouseID GetID() const {
        return id;
    }

    void Poll() override {
        state = SDL_GetMouseState(&x, &y);
    }

    bool IsActiveThisFrame() const override { return active; }

    bool IsDown(int button) const override {
        return (state & SDL_BUTTON_MASK(button)) != 0;
    }

    float GetAxis(int axis) const override {
        return (axis == 0) ? x : (axis == 1) ? y : 0.0f;
    }
};

class GamepadDevice : public InputDevice {
    SDL_Gamepad* gamepad;
    SDL_JoystickID id;
    bool active = false;

public:
    GamepadDevice(SDL_JoystickID instanceID) : id(instanceID) {
        gamepad = SDL_OpenGamepad(instanceID);
    }
    ~GamepadDevice() {
        SDL_CloseGamepad(gamepad);
    }

    DeviceType GetType() const override {
        return DeviceType::Gamepad;
    }

    SDL_JoystickID GetID() const {
        return id;
    }

    void Poll() override 
    {
        active = false;

        for (int i = 0; i < SDL_GAMEPAD_BUTTON_COUNT;i++) {
            if (SDL_GetGamepadButton(gamepad, (SDL_GamepadButton)i)) 
            {
                active = true;
                break;
            }
        }
    }

    bool IsActiveThisFrame() const override {
        return active;
    }

    bool IsDown(int button) const override {
        return SDL_GetGamepadButton(gamepad, (SDL_GamepadButton)button);
    }

    float GetAxis(int axis) const override {
        return SDL_GetGamepadAxis(gamepad, (SDL_GamepadAxis)axis) / 32767.0f;
    }
};

class JoystickDeviceSDL : public InputDevice {
    SDL_Joystick* joystick;
    SDL_JoystickID id;
    bool active = false;

public:
    JoystickDeviceSDL(SDL_JoystickID instanceID) : id(instanceID) {
        joystick = SDL_OpenJoystick(instanceID);
    }

    ~JoystickDeviceSDL() {
        SDL_CloseJoystick(joystick);
    }

    DeviceType GetType() const override {
        return DeviceType::Joystick;
    }

    SDL_JoystickID GetID() const {
        return id;
    }

    void Poll() override {
        active = false;
        int numButtons = SDL_GetNumJoystickButtons(joystick);
        for (int i = 0; i < numButtons;i++) {
            if (SDL_GetJoystickButton(joystick, i)) 
            {
                active = true;
                break;
            }
        }
    }

    bool IsActiveThisFrame() const override {
        return active;
    }

    bool IsDown(int button) const override {
        return SDL_GetJoystickButton(joystick, button);
    }

    float GetAxis(int axis) const override {
        return SDL_GetJoystickAxis(joystick, axis) / 32767.0f;
    }
};


 
class Keyboard
{
public:
    static InputDevice& Current();
    static const std::vector<InputDevice*>& All();

private:
    friend class InputSystem;
    static std::vector<InputDevice*> Devices;
    static InputDevice* CurrentDevice;
};

class Mouse
{
public:
    static InputDevice& Current();
    static const std::vector<InputDevice*>& All();

private:
    friend class InputSystem;
    static std::vector<InputDevice*> Devices;
    static InputDevice* CurrentDevice;
};

class Gamepad
{
public:
    static InputDevice& Current();
    static const std::vector<InputDevice*>& All();

private:
    friend class InputSystem;
    static std::vector<InputDevice*> Devices;
    static InputDevice* CurrentDevice;
};
