#include "State.h"
#include "SharedDataRef.h"

void State::SetData()
{
	_data->physics.Tie(ecs); ecs.Tie(_data);
}