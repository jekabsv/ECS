#pragma once

#include <memory>
#include <stack>
#include "State.h"

typedef std::unique_ptr<State> StateRef;

class StateMachine
{
public:
    StateMachine() : _isRemoving(0), _isAdding(0), _isreplacing(0) {}
    ~StateMachine() {}

    void AddState(StateRef newState, bool isReplacing);
    void RemoveState();
    void ProcessStateChanges();

    StateRef& GetActiveState();

private:
    std::stack<StateRef> _states;
    StateRef _newState;
    bool _isRemoving, _isAdding, _isreplacing;
};
