#include "State.h"
#include "SharedDataRef.h"

void State::SetData()
{
	_data->physics.Tie(ecs);
	ecs.Tie(_data);
	ui.Init(_data->SDLrenderer, (float)_data->GAME_WIDTH, (float)_data->GAME_HEIGHT, _data->window);
}