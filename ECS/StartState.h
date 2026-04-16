#pragma once
#include "SharedDataRef.h"
#include "State.h"

class StartState : public State
{
public:
    using State::State;
    void Init() override;
	void Update(float dt) override;


    UI::NodeHandle btnBoids = UI::NULL_HANDLE;
    UI::NodeHandle btnSillyGame = UI::NULL_HANDLE;
    UI::NodeHandle btnSlider = UI::NULL_HANDLE;
    UI::NodeHandle btnQuit_ = UI::NULL_HANDLE;

    float rotationAngle = 0.0f;
    int x = 0;
    int y = 0;
};



