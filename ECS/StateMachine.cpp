#include "StateMachine.h"
#include "State.h"
#include "logger.h"


StateMachine::StateMachine() : _isRemoving(false), _isAdding(false), _isReplacing(false) {}

StateMachine::~StateMachine() = default;

void StateMachine::AddState(StateRef newState, bool isreplacing)
{
	_isAdding = 1;
	_isReplacing = isreplacing;
	_newState = std::move(newState);
}
int StateMachine::RemoveState()
{
	if (_states.empty()) {
		LOG_WARN(GlobalLogger(), "StateMachine", "RemoveState called on empty stack");
		return -1;
	}

	_isRemoving = true;
	return 0;
}


void StateMachine::ProcessStateChanges()
{
	if (_isRemoving && !_states.empty())
	{
		_states.pop();
		if (!_states.empty())
			_states.top()->Resume();
		_isRemoving = false;
	}
	if (_isAdding)
	{
		if (_isReplacing && !_states.empty())
			_states.pop();
		else if (!_states.empty())
			_states.top()->Pause();

		_states.push(std::move(_newState));
		_isAdding = false;

		this->_states.top()->SetData();
		this->_states.top()->Init();

		LOG_DEBUG(GlobalLogger(), "StateMachine", "State transition completed");
	}
}

StateRef& StateMachine::GetActiveState()
{
	if (_states.empty())
	{
		static StateRef errorState = nullptr;
		LOG_ERROR(GlobalLogger(), "StateMachine", "GetActiveState called on empty stack");
		return errorState;
	}
	return _states.top();
}