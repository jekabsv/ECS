#pragma once
#include "SharedDataRef.h"
#include "State.h"

class StartState : public State
{
public:
    using State::State;
    void Init() override;
    void HandleInput() override {}

    UI::NodeHandle btnPlay_ = UI::NULL_HANDLE;
    UI::NodeHandle btnQuit_ = UI::NULL_HANDLE;
};



