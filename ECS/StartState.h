#pragma once
#include "SharedDataRef.h"
#include "State.h"

class StartState : public State
{
public:
    StartState(SharedDataRef data) : _data(data) {}

    void Init() override;
    void HandleInput() override {}

private:
    SharedDataRef _data;
};



