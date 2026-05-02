#include "State.h"
#include "SharedDataRef.h"
#include <iostream>

void State::SetData()
{
	LOG_DEBUG(GlobalLogger(), "State", "Setting state data");

	_data->physics.Tie(ecs);
	ecs.Tie(_data);


	MaterialBase base;
	MaterialBase::MakeTransparent(base);
	MaterialBase::SetVertexAttr(base);
	base.vertexShader = "sprite_vert";
	base.fragmentShader = "sprite_frag";


	ui.Init(&_data->renderer, (float)_data->GAME_WIDTH, (float)_data->GAME_HEIGHT, _data->window, &_data->assets);

	std::cout << "State data set\n";
}