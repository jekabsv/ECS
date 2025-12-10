#pragma once
#include "StateMachine.h"
#include <iostream>
#include "SharedDataRef.h"

class StartState : public State
{
public:
    StartState(SharedDataRef data) : _data(data) {}

    void Init() override;
    void HandleInput() override {}
    void Update() override;
    void Render() override;

private:
    SharedDataRef _data;
};



