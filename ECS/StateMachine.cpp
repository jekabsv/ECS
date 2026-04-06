#include "StateMachine.h"
#include <cassert>
#include "logger.h"
#include "SharedDataRef.h"

void StateMachine::AddState(StateRef newState, bool isreplacing)
{
	_isAdding = 1;
	_isreplacing = isreplacing;
	_newState = std::move(newState);
}
void StateMachine::RemoveState()
{
	if (_states.empty()) {
		LOG_WARN(GlobalLogger(), "StateMachine", "RemoveState called on empty stack"); return; 
	}

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
		_isAdding = 0;
		this->_states.top()->_data->physics.Tie(this->_states.top()->ecs, this->_states.top()->_data);
		this->_states.top()->ecs.Tie(this->_states.top()->_data);
		this->_states.top()->Init();
		LOG_DEBUG(GlobalLogger(), "StateMachine", "State transition completed");		
	}
}

StateRef& StateMachine::GetActiveState()
{
	if (_states.empty())
		LOG_ERROR(GlobalLogger(), "StateMachine", "GetActiveState called on empty stack");
	return _states.top();
}
