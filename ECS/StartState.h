#pragma once
#include "SharedDataRef.h"
#include "State.h"

class StartState : public State
{
public:
    using State::State;
    void Init() override;
	void Update(float dt) override;
	void Render(float dt) override;

    UI::Context ui;
    UI::NodeHandle btnTest;
    UI::NodeHandle sliderVol;

    float rotationAngle = 0.0f;
    float x = 400.f;
    float y = 300.f;
};



