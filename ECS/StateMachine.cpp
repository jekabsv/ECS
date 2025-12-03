#include "StateMachine.h"
#include <cassert>

void StateMachine::AddState(StateRef newState, bool isreplacing)
{
	_isAdding = 1;
	_isreplacing = isreplacing;
	_newState = std::move(newState);
}
void StateMachine::RemoveState()
{
	_isRemoving = 1;
}

void StateMachine::ProcessStateChanges()
{
	if (_isRemoving && !_states.empty())
	{
		_states.pop();
		if (!_states.empty())
		{
			_states.top()->Resume();
		}
		_isRemoving = 0;
	}
	if (_isAdding)
	{
		if (_isreplacing && !_states.empty())
			_states.pop();
		else if (!_states.empty())
		{ 
			_states.top()->Pause();
		}
		_states.push(std::move(_newState));
		this->_states.top()->Init();
		_isAdding = 0;
	}
}

StateRef& StateMachine::GetActiveState()
{
	return _states.top();
}
